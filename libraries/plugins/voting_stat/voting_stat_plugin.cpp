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

   voting_stat_plugin& _self;
   
   graphene::chain::database& database()
   {
      return _self.database();
   }

   /**
    * @brief callback for database::stake_calculated
    * updates the given stake anc proxy account
    */
   void on_stake_calculated(
      const account_object& stake_account,
      const account_object& proxy_account, 
      uint64_t stake );

   /**
    * @brief callback for database::maintenance_begin
    * updated the block_id to the one where the maintenance interval occurs
    */
   void on_maintenance_begin( const block_id_type& block_id );

private:
   void create_voting_statistics_for_stake_account( 
      const account_object& stake_account, 
      const account_id_type proxy_id, 
      uint64_t stake );
   
   void modify_voting_statistics_for_stake_account(
      const voting_statistics_object& stake_stat_obj,
      const account_object& stake_account,
      const account_id_type proxy_id,
      uint64_t stake );
   
   void create_voting_statistics_for_proxy_account(
      const account_id_type stake_id,
      const account_id_type proxy_id,
      uint64_t proxied_stake );

   void insert_proxied_stake_into_proxy_account(
      const voting_statistics_object& proxy_stat_obj,
      const account_id_type stake_id,
      uint64_t proxied_stake );

   void delete_proxied_stake_from_proxy_account(
      const voting_statistics_object& proxy_stat_obj,
      const account_id_type stake_id );
};

void voting_stat_plugin_impl::on_maintenance_begin( const block_id_type& block_id )
{
   graphene::chain::voting_statistics_object::block_id = block_id;
}

void voting_stat_plugin_impl::on_stake_calculated( 
   const account_object& stake_account,
   const account_object& proxy_account, 
   uint64_t stake )
{
   auto& db = database();

   account_id_type stake_id = stake_account.id;
   account_id_type new_proxy_id = stake_account.id == proxy_account.id ?
      GRAPHENE_PROXY_TO_SELF_ACCOUNT : static_cast<account_id_type>(proxy_account.id);
   account_id_type old_proxy_id;

   bool has_stake_changed;
   
   const auto& idx = db.get_index_type<voting_statistics_index>().indices().get<by_owner>();

   auto stake_stat_it = idx.find( stake_id );
   if( stake_stat_it == idx.end() )
   {
      has_stake_changed = true;
      old_proxy_id      = GRAPHENE_PROXY_TO_SELF_ACCOUNT;
      create_voting_statistics_for_stake_account( stake_account, new_proxy_id, stake );
      stake_stat_it = idx.find( stake_id );
   }
   else
   {
      has_stake_changed = stake_stat_it->stake != stake;
      old_proxy_id      = stake_stat_it->proxy;

      modify_voting_statistics_for_stake_account( *stake_stat_it, stake_account,
         new_proxy_id, stake );
   }
   
   if( old_proxy_id != new_proxy_id )
   {
      if( old_proxy_id != GRAPHENE_PROXY_TO_SELF_ACCOUNT ) 
      {
         auto old_proxy_stat_it = idx.find( old_proxy_id );
         delete_proxied_stake_from_proxy_account( *old_proxy_stat_it, stake_id );
      }
      if( new_proxy_id != GRAPHENE_PROXY_TO_SELF_ACCOUNT )
      {
         auto proxy_stat_it = idx.find( new_proxy_id );
         if( proxy_stat_it == idx.end() ) 
         {
            create_voting_statistics_for_proxy_account( stake_id, new_proxy_id,
               stake );
         }
         else if( proxy_stat_it->account != stake_id ) 
         {
            insert_proxied_stake_into_proxy_account( *proxy_stat_it, stake_id,
               stake );
         }
      }
   }
   else if( has_stake_changed && stake_stat_it->has_proxy() )
   {
      auto proxy_stat_it = idx.find( proxy_account.id );
      insert_proxied_stake_into_proxy_account( *proxy_stat_it, stake_id,
         stake );
   }
}

void voting_stat_plugin_impl::create_voting_statistics_for_stake_account( 
   const account_object& stake_account, 
   const account_id_type proxy_id, 
   uint64_t stake )
{
   database().create<voting_statistics_object>(
      [&stake_account, proxy_id, stake] (voting_statistics_object& o){
         o.account = stake_account.id;
         o.stake   = stake;
         o.proxy   = proxy_id;
         o.votes   = stake_account.options.votes;
      }
   );
}

void voting_stat_plugin_impl::modify_voting_statistics_for_stake_account(
   const voting_statistics_object& stake_stat_obj,
   const account_object& stake_account,
   const account_id_type proxy_id,
   uint64_t stake )
{
   database().modify<voting_statistics_object>(stake_stat_obj,
      [&stake_account, proxy_id, stake] (voting_statistics_object& o) {
         o.stake = stake;
         o.proxy = proxy_id;
         o.votes = stake_account.options.votes;
      }
   ); 
}

void voting_stat_plugin_impl::create_voting_statistics_for_proxy_account(
   const account_id_type stake_id,
   const account_id_type proxy_id,
   uint64_t proxied_stake )
{
   database().create<voting_statistics_object>(
      [stake_id, proxy_id, proxied_stake] (voting_statistics_object& o) {
         o.account = proxy_id;
         o.proxy_for.emplace( stake_id, proxied_stake ); 
      }
   );
}

void voting_stat_plugin_impl::insert_proxied_stake_into_proxy_account(
   const voting_statistics_object& proxy_stat_obj,
   const account_id_type stake_id,
   uint64_t proxied_stake )
{
   database().modify<voting_statistics_object>( proxy_stat_obj,
      [stake_id, proxied_stake] (voting_statistics_object& o) {
         auto insertion_return = o.proxy_for.emplace( stake_id, proxied_stake );
         if( !insertion_return.second )
            insertion_return.first->second = proxied_stake;
      }
   );
}

void voting_stat_plugin_impl::delete_proxied_stake_from_proxy_account(
   const voting_statistics_object& proxy_stat_obj,
   const account_id_type stake_id )
{
   database().modify<voting_statistics_object>( proxy_stat_obj, 
      [stake_id] (voting_statistics_object& o) {
         o.proxy_for.erase( stake_id ); 
      }
   );
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
   boost::program_options::options_description& cfg
   )
{
}

void voting_stat_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   auto& db = database();
   db.add_index< primary_index< voting_statistics_index > >();

   db.voting_stake_calculated.connect( 
      [&](const account_object& stake_account, const account_object& proxy_account, 
         const uint64_t stake) {
            my->on_stake_calculated( stake_account, proxy_account, stake ); 
         }
   );
   
   db.begin_maintenance_interval.connect(
      [&](const block_id_type& block_id){ my->on_maintenance_begin( block_id ); });

}

void voting_stat_plugin::plugin_startup()
{
}

} }
