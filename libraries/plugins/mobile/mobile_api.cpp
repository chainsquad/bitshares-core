#include <graphene/app/application.hpp>

#include <graphene/chain/database.hpp>

#include <graphene/mobile/mobile_api.hpp>
#include <graphene/mobile/mobile_plugin.hpp>
#include <graphene/mobile/mobile_account.hpp>

namespace graphene { namespace mobile {

/*
 * The detail is used to separated the implementation from the interface
 */
namespace detail {

class mobile_api_impl
{
   public:
      mobile_api_impl( graphene::app::application& _app );
      graphene::app::application& app;
      std::shared_ptr< graphene::mobile_plugin::mobile_plugin > get_plugin();

      std::map<string,mobile_account> get_mobile_accounts( const vector<string>& names_or_ids );
      vector<mobile_account_balance_object> get_mobile_balances( const string& name_or_id );
};

/**
 * Constructor Implementation
 */
mobile_api_impl::mobile_api_impl( graphene::app::application& _app ) : app( _app )
{}

/**
 * Get plugin name Implementation
 */
std::shared_ptr< graphene::mobile_plugin::mobile_plugin > mobile_api_impl::get_plugin()
{
   /// We are loading the mobile_plugin's get_plugin all here
   return app.get_plugin< graphene::mobile_plugin::mobile_plugin >( "mobile" );
}

/**
 * Custom call call implementations
 */
std::map<std::string, mobile_account> mobile_api_impl::get_mobile_accounts( const vector<std::string>& names_or_ids)
{
   idump((names_or_ids));
   std::map<std::string, mobile_account> results;
   // Obtain access to the internal database objects
   std::shared_ptr< graphene::chain::database > _db = app.chain_database();

   for (const std::string& account_name_or_id : names_or_ids)
   {
      const account_object* account = nullptr;
      if (std::isdigit(account_name_or_id[0]))
         account = _db->find(fc::variant(account_name_or_id).as<account_id_type>());
      else
      {
         const auto& idx = _db->get_index_type<account_index>().indices().get<by_name>();
         auto itr = idx.find(account_name_or_id);
         if (itr != idx.end())
            account = &*itr;
      }
      if (account == nullptr)
         continue;

      // Account
      mobile_account acnt;

      // Account itself
      mobile_account_object account_object;
      account_object.name = account->name;
      account_object.owner = account->owner;
      account_object.active = account->active;
      account_object.options = account->options;
      acnt.account = account_object;

      // Add the account's balances
      auto balance_range = _db->get_index_type<account_balance_index>().indices().get<by_account_asset>().equal_range(boost::make_tuple(account->id));
      //vector<account_balance_object> balances;
      std::for_each(balance_range.first, balance_range.second,
                    [&acnt, &_db](const account_balance_object& balance) {
                       // get corresponding asset
                       asset_object balance_asset = _db->get(balance.asset_type);
                       // construct mobile account balance object
                       mobile_account_balance_object mobile_balance;
                       mobile_balance.balance = balance.balance;
                       mobile_balance.precision = balance_asset.precision;
                       mobile_balance.symbol = balance_asset.symbol;
                       acnt.balances.emplace_back(mobile_balance);
                    });
      results[account_name_or_id] = acnt;
   }
   return results;
}

vector<mobile_account_balance_object> mobile_api_impl::get_mobile_balances( const string& name_or_id )
{
   // Return object
   vector<mobile_account_balance_object> balances;

   // Obtain access to the internal database objects
   std::shared_ptr< graphene::chain::database > _db = app.chain_database();

   const account_object* account = nullptr;
   if (std::isdigit(name_or_id[0]))
      account = _db->find(fc::variant(name_or_id).as<account_id_type>());
   else
   {
      const auto& idx = _db->get_index_type<account_index>().indices().get<by_name>();
      auto itr = idx.find(name_or_id);
      if (itr != idx.end())
         account = &*itr;
   }
   if (account == nullptr)
      return balances;

   // Add the account's balances
   auto balance_range = _db->get_index_type<account_balance_index>().indices().get<by_account_asset>().equal_range(boost::make_tuple(account->id));
   //vector<account_balance_object> balances;
   std::for_each(balance_range.first, balance_range.second,
                 [&balances, &_db](const account_balance_object& balance) {
                    // get corresponding asset
                    asset_object balance_asset = _db->get(balance.asset_type);
                    // construct mobile account balance object
                    mobile_account_balance_object mobile_balance;
                    mobile_balance.balance = balance.balance;
                    mobile_balance.precision = balance_asset.precision;
                    mobile_balance.symbol = balance_asset.symbol;
                    balances.emplace_back(mobile_balance);
                 });
   return balances;
}


} /// detail

/*
 * Now we are done with implementation/detail, let's do the interface (linking
 * implementation)
 */

/*
 * Plugin constructor implementation
 */
mobile_api::mobile_api( graphene::app::application& app )
   : my( new detail::mobile_api_impl( app ) ) {}

/*
 * Custom call 'mobile' interface
 */
std::map<string,mobile_account> mobile_api::get_mobile_accounts( const vector<string>& names_or_ids )
{
   return my->get_mobile_accounts( names_or_ids );
}

vector<mobile_account_balance_object> mobile_api::get_mobile_balances( const string& name_or_id )
{
   return my->get_mobile_balances( name_or_id );
}

} } // graphene::mobile
