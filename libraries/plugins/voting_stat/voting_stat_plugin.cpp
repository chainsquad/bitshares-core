/*
 * Copyright (c) 2019 Blockchain Projects BV.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <graphene/voting_stat/voting_stat_plugin.hpp>

#include <graphene/chain/voting_statistics_object.hpp>
#include <graphene/chain/worker_object.hpp>
#include <graphene/chain/voteable_statistics_object.hpp>

#include <graphene/chain/protocol/config.hpp>

#include <boost/signals2/connection.hpp>
#include <boost/signals2/shared_connection_block.hpp>

#include <memory>

namespace graphene { namespace voting_stat {

namespace detail {

class voting_stat_plugin_impl
{
public:
   voting_stat_plugin_impl(voting_stat_plugin& _plugin)
      : _self( _plugin ){}

   virtual ~voting_stat_plugin_impl(){}

   boost::signals2::connection _on_voting_stake_calc_conn;
   std::unique_ptr<boost::signals2::shared_connection_block> 
      _on_voting_stake_calc_block;
   uint8_t _maint_counter = 0;   
   
   uint8_t _track_every_x_maint = 12;
   bool    _del_objs_after_maint = true;

   /**
    * @brief callback for database::on_stake_calculated
    * updates the given stake anc proxy account
    */
   void on_stake_calculated(
      const account_object& stake_account,
      const account_object& proxy_account, 
      uint64_t stake );

   /**
    * @brief callback for database::on_maintenance_begin
    * updates the block_num to the one where the maintenance interval occurs
    * and connects the on_stake_calculated callback so that statistics objects
    * will be created 
    */
   void on_maintenance_begin( uint32_t block_num );

   /**
    * @brief callback for database::on_maintenance_end
    * disconnects the on_stake_calculated callback and deletes all statistics
    * objects if _voting_stat_delete_objects_after_interval=true
    */
   void on_maintenance_end();

   void create_voteable_statistic_objects();
   void delete_all_statistics_objects();

   graphene::chain::database& database()
   {
      return _self.database();
   }

private:
   voting_stat_plugin& _self;
   
   uint32_t _maint_block;
};

void voting_stat_plugin_impl::on_maintenance_begin( uint32_t block_num )
{
   if( _maint_counter == _track_every_x_maint )  
   {
      _maint_counter = 0;
      _maint_block = block_num;
      _on_voting_stake_calc_block->unblock();
   }
   ++_maint_counter;
}

void voting_stat_plugin_impl::on_maintenance_end()
{
   _on_voting_stake_calc_block->block();

   create_voteable_statistic_objects();

   if( _del_objs_after_maint ) 
      delete_all_statistics_objects();
}

void voting_stat_plugin_impl::delete_all_statistics_objects()
{
   auto& db = database();

   const auto& voting_idx = db.get_index_type<voting_statistics_index>().indices();
   for( const auto& voting_obj : voting_idx ) {
      db.remove( voting_obj );
   }
   
   const auto& voteable_idx = db.get_index_type<voteable_statistics_index>().indices();
   for( const auto& voteable_obj : voteable_idx ) {
      db.remove( voteable_obj );
   }
}

void voting_stat_plugin_impl::create_voteable_statistic_objects()
{
   auto& db = database();
 
   // TODO create variable for tracker_worker, track_witness, track_committee


   // we have first to create all voteable statistics for each voteable type
   // create all workerstatistics
   // TODO secondary index for workers where current_time < worker_end_time

   // TODO IF TRACK_WORKER
   const auto& worker_idx = db.get_index_type<worker_index>().indices().get<by_id>();
   
   auto now = db.head_block_time();
   for( const auto& worker : worker_idx )
   {
      if( now > worker.work_end_date ) 
         continue;

      db.create<voteable_statistics_object>( 
         [this, &worker]( voteable_statistics_object& o )
         {
            o.block_number = this->_maint_block;
            o.vote_id = worker.vote_for;
         }
      );
   }   

   // TODO SAME FOR WITNESS AND COMMITTEE




   const auto& voting_stat_idx = db.get_index_type<voting_statistics_index>().indices()
      .get<by_owner>();
   const auto& voteable_idx = db.get_index_type<voteable_statistics_index>().indices()
      .get<by_vote_id>();

   for( const auto& stat_obj : voting_stat_idx )
   {
      uint64_t total_stake = stat_obj.get_total_voting_stake();
      if( !total_stake )
         continue; // don't bother inserting a 0 stake
      
      const flat_set<vote_id_type>& votes = stat_obj.votes;
      for( const auto& vote_id : votes )
      {
         auto voteable_obj_it = voteable_idx.find( vote_id );
         if( voteable_obj_it == voteable_idx.end() ) 
            continue; // if an obj is not found it is unwanted

         const auto& voteable_obj = *voteable_obj_it;
         db.modify<voteable_statistics_object>( voteable_obj,
            [this, vote_id, stat_obj, total_stake]
               ( voteable_statistics_object& o )
               {
                  o.block_number = this->_maint_block;
                  o.vote_id = vote_id;
                  o.voted_by.emplace( stat_obj.account, total_stake );
               }  
         );
      }
   }
}

void voting_stat_plugin_impl::on_stake_calculated( 
   const account_object& stake_account,
   const account_object& proxy_account, 
   uint64_t stake )
{
   auto& db = database();

   account_id_type stake_id = stake_account.id;
   account_id_type proxy_id = proxy_account.id;
   proxy_id = ( stake_id == proxy_id ? GRAPHENE_PROXY_TO_SELF_ACCOUNT : proxy_id );
   
   const auto& idx = db.get_index_type<voting_statistics_index>().indices().get<by_owner>();
   auto stake_stat_it = idx.find( stake_id );
   if( stake_stat_it == idx.end() )
   {
      /* if stake account doesn't exist, create it */
      db.create<voting_statistics_object>(
         [this, &stake_account, &proxy_id, stake] 
            (voting_statistics_object& o)
            {
               o.block_number = this->_maint_block;
               o.account = stake_account.id;
               o.stake   = stake;
               o.proxy   = proxy_id;
               o.votes   = stake_account.options.votes;
            }
      );
   }
   else
   {
      /* if stake account exists, modify it */
      db.modify<voting_statistics_object>( *stake_stat_it, 
         [this, &stake_account, &proxy_id, stake] 
            (voting_statistics_object& o) 
            {
               o.block_number = this->_maint_block;
               o.stake = stake;
               o.proxy = proxy_id;
               o.votes = stake_account.options.votes;
            }
      ); 
   }
   
   if( proxy_id != GRAPHENE_PROXY_TO_SELF_ACCOUNT )
   {
      auto proxy_stat_it = idx.find( proxy_id );
      if( proxy_stat_it == idx.end() ) 
      {
         /* if the new proxy account doesn't exist, create and add the proxied stake */
         db.create<voting_statistics_object>(
            [&stake_id, &proxy_id, stake] (voting_statistics_object& o) {
               o.account = proxy_id;
               o.proxy_for.emplace( stake_id, stake ); 
            }
         );
      }
      else
      {
         /* insert the stake into the proxy account */
         db.modify<voting_statistics_object>( *proxy_stat_it,
            [&stake_id, stake] (voting_statistics_object& o) 
            {
               auto insertion_return = o.proxy_for.emplace( stake_id, stake );
               FC_ASSERT( insertion_return.second, 
                  "the stake could not be inserted in the stake account. THIS SHOULD NORMALLY NOT HAPPEN" ); // TODO REMOVE THIS
               if( !insertion_return.second ) 
                  insertion_return.first->second = stake;
            }
         );
      }
   }
}

} // namespace::detail



voting_stat_plugin::voting_stat_plugin() :
   my( new detail::voting_stat_plugin_impl(*this) )
{
}

voting_stat_plugin::~voting_stat_plugin()
{
}

std::string voting_stat_plugin::plugin_name()const
{
   return "voting_stat";
}

void voting_stat_plugin::plugin_set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg )
{
   cli.add_options()
      (
         "voting-stat-track-every-x-maint", 
         boost::program_options::value<uint8_t>(), 
         "Every x maintenance interval statistic objects will be created (12=2per day)"
      )
      (
         "voting-stat-del-objs-after-maint", 
         boost::program_options::value<bool>(), 
         "Every created object will be deleted after the maintenance interval (true)" 
      );
   cfg.add(cli);
}

void voting_stat_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   auto& db = database();
   db.add_index< primary_index< voting_statistics_index > >();
   db.add_index< primary_index< voteable_statistics_index > >();

   if( options.count("voting-stat-track-every-x-maint") ){
      my->_track_every_x_maint = options["voting-stat-track-every-x-maint"].as<uint8_t>();
      my->_maint_counter = my->_track_every_x_maint;
   }
   if( options.count("voting-stat-del-objs-after-maint") ){
      my->_del_objs_after_maint = 
         options["voting-stat-del-objs-after-maint"].as<bool>();
   }

   my->_on_voting_stake_calc_conn = 
      db.on_voting_stake_calculated.connect( 
         [&](const account_object& stake_account, const account_object& proxy_account, 
            const uint64_t stake) {
               my->on_stake_calculated( stake_account, proxy_account, stake ); 
            }
      );

   my->_on_voting_stake_calc_block = 
      std::unique_ptr<boost::signals2::shared_connection_block>( 
         new boost::signals2::shared_connection_block( my->_on_voting_stake_calc_conn )
      );

   db.on_maintenance_begin.connect(
      [&](uint32_t block_num){ my->on_maintenance_begin( block_num ); }
   );

   db.on_maintenance_end.connect( 
      [&](){ my->on_maintenance_end(); }
   );
}

void voting_stat_plugin::plugin_startup()
{
}

} }
