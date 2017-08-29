#pragma once

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>

namespace graphene {
namespace mobile {

   using namespace graphene::chain;

   /**
    * @brief Tracks the balance of a single account/asset pair
    * @ingroup object
    */
   struct mobile_account_object
   {
      string            name;
      authority         owner;
      authority         active;
      account_options   options;
   };

   struct mobile_account_balance_object
   {
      share_type        balance;
      string            symbol;
      uint8_t           precision = 0;
   };

   struct mobile_account
   {
      mobile_account_object                   account;
      vector<mobile_account_balance_object>   balances;
   };

}}

FC_REFLECT( graphene::mobile::mobile_account,
            (account)
            (balances)
          );
FC_REFLECT( graphene::mobile::mobile_account_object,
            (name)(owner)(active)(options)
          );

FC_REFLECT( graphene::mobile::mobile_account_balance_object,
            (balance)(symbol)(precision)
          );
