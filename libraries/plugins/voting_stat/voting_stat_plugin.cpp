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

#include <graphene/chain/protocol/config.hpp>

namespace graphene { namespace voting_stat {

namespace detail {

class voting_stat_plugin_impl
{
public:
   voting_stat_plugin_impl(voting_stat_plugin& _plugin)
      : _self( _plugin ){}

   virtual ~voting_stat_plugin_impl(){}

   boost::signals2::connection _on_voting_stake_calculated_connection;
   std::unique_ptr<boost::signals2::shared_connection_block> 
      _on_voting_stake_calculated_block;
   uint8_t _maintenance_counter = 0;   
   
   uint8_t _track_every_x_maintenance_interval = 12;
   bool    _delete_objects_after_maintenance_interval = true;

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

   graphene::chain::database& database()
   {
      return _self.database();
   }

private:
   voting_stat_plugin& _self;
   
   uint32_t _maintenance_block;
};

void voting_stat_plugin_impl::on_maintenance_begin( uint32_t block_num )
{
   if( _maintenance_counter == _track_every_x_maintenance_interval )  
   {
      _maintenance_counter = 0;
      _maintenance_block = block_num;
      _on_voting_stake_calculated_block->unblock();
   }
   ++_maintenance_counter;
}

void voting_stat_plugin_impl::on_maintenance_end()
{
   _on_voting_stake_calculated_block->block();
   if( _delete_objects_after_maintenance_interval ) 
   {
      const auto& idx = db.get_index_type<voting_statistics_index>().indices();
      for( auto& id : idx ) 
      {
         // TODO DELETE ALL OBJECTS
      }   
   }
}

void voting_stat_plugin_impl::create_voteable_statistic_objects()
{
   auto& db = database();

   // we have first to create all voteable statistics for each voteable type
   // create all workerstatistics
   // TODO secondary index for workers where current_time < worker_end_time
   const auto& worker_idx = db.get_index_type<worker_index>().indices().get<by_id>();
   




   const auto& idx = db.get_index_type<voting_statistics_index>().indices().get<by_owner>();
   const auto& voteable_idx = db.get_index_type<voteable_statistics_index>().indices().get<by_vote_id>();
   



   for( auto stat_obj_it = idx.begin(); stat_obj_it != idx.end(); ++stat_obj_it ) // iterate over every stat obj
   {
      auto total_stake = stat_obj_it->get_total_voting_stake();
      if( total_stake == 0 )
         continue; // don't bother inserting a 0 stake
      
      auto& votes = stat_obj_it->votes;
      for( auto vote_it = votes.begin(); vote_it != votes.end(); ++vote_it )
      {
         auto voteable_obj_it = voteable_idx.find( *vote_it );
         if( voteable_obj_it == voteable_idx.end() ) 
         {
            db.create<voteable_statistics_object>( 
               [_maintenance_block, vote_it, stat_obj_it, total_stake]( voteable_statistics_object& o )
               {
                  o.block_number = _maintenance_block;
                  o.vote_id = *vote_it;
                  o.voted_by.emplace( stat_obj_it->account, total_stake );
               }  
            );
         }
         else
         {
            // update obj by inserting everything
         }
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
      db.create<voting_statistics_object>(
         [&stake_account, &proxy_id, stake, _maintenance_block] 
            (voting_statistics_object& o)
            {
               o.block_number = _maintenance_block;
               o.account = stake_account.id;
               o.stake   = stake;
               o.proxy   = &proxy_id;
               o.votes   = stake_account.options.votes;
            }
      );
   }
   else
   {
      db.modify<voting_statistics_object>( *stake_stat_it, 
         [&stake_account, &proxy_id, _maintenance_block, stake] 
            (voting_statistics_object& o) 
            {
               o.block_number = _maintenance_block;
               o.stake = stake;
               o.proxy = proxy_id;
               o.votes = stake_account.options.votes;
            }
      ); 
   }
   
   if( proxy != GRAPHENE_PROXY_TO_SELF_ACCOUNT )
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
                  "the stake could not be inserted in the stake account. THIS
                  SHOULD NORMALLY NOT HAPPEN" ); // TODO REMOVE THIS
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
         "voting-stat-track-every-x-maintenance-interval", 
         boost::program_options::value<uint8_t>(), 
         "Every x maintenance interval statistic objects will be created (12=2per day)"
      )
      (
         "voting-stat-delete-objects-after-maintenance-interval", 
         boost::program_options::value<bool>(), 
         "Every created object will be deleted after the maintenance interval (true)" 
      );
   cfg.add(cli);
}

void voting_stat_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   auto& db = database();
   db.add_index< primary_index< voting_statistics_index > >();
   
   if( options.count("voting-stat-track-every-x-maintenance-interval") ){
      my->_track_every_x_maintenance_interval
         = options["voting-stat-track-every-x-maintenance-interval"].as<uint8_t>();
      my->_maintenance_counter = my->_track_every_x_maintenance_interval;
   }
   if( options.count("voting-stat-delete-objects-after-maintenance-interval") ){
      my->_delete_objects_after_maintenance_interval 
         = options["voting-stat-delete-objects-after-maintenance-interval"].as<bool>();
   }

   my->_on_voting_stake_calculated_connection = db.on_voting_stake_calculated.connect( 
      [&](const account_object& stake_account, const account_object& proxy_account, 
         const uint64_t stake) {
            my->on_stake_calculated( stake_account, proxy_account, stake ); 
         }
   );

   my->_on_voting_stake_calculated_block = std::make_unique<boost::signals2::shared_connection_block>( 
      my->_on_voting_stake_calculated_connection 
   );

   db.on_maintenance_begin.connect(
      [&](uint32_t block_num){ my->on_maintenance_begin( block_num ); }
   );

   db.on_maintenance_end.connect( 
      [&](){ my->on_maintenance_end() };
   );
}

void voting_stat_plugin::plugin_startup()
{
}

} }
