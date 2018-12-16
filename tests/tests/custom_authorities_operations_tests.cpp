/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
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

#include <boost/test/unit_test.hpp>
#include <graphene/chain/protocol/custom_authority.hpp>
#include <graphene/chain/custom_authority_object.hpp>
#include <graphene/chain/operation_type_to_id.hpp>

#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;
using namespace graphene::app;

namespace {
   vector< custom_authority_object > get_custom_authorities_by_account( database& db, account_id_type account )
   {
      const auto& authority_by_account = db.get_index_type<custom_authority_index>().indices().get<by_account>();
      
      vector<custom_authority_object> result;
      
      auto itr = authority_by_account.find(account);
      while(itr != authority_by_account.end())
      {
         result.emplace_back(*itr++);
      }
      
      return result;
   }
   
}

BOOST_FIXTURE_TEST_SUITE( custom_authorities_operations, database_fixture )

BOOST_AUTO_TEST_CASE(get_custom_authorities_by_account_without_authorities) {
   try {
      auto dan = create_account("dan");
      
      generate_block();
      
      BOOST_CHECK(get_custom_authorities_by_account(db, dan.id).empty());
   } catch (fc::exception &e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(get_custom_authorities_by_account_without_authorities_but_with_authorities_for_another_account) {
   try {
      auto dan = create_account("dan");
      auto sam = create_account("sam");
      
      generate_block();
      fc::usleep(fc::milliseconds(2000));
      
      custom_authority_create_operation op;
      op.account = sam.id;
      op.enabled = true;
      op.valid_from = time_point_sec(1);
      op.valid_to = time_point_sec(2);
      op.operation_type = operation_type_id_from_operation_type<transfer_operation>::value;
      
      eq_restriction rest;
      rest.argument = "amount";
      rest.value = asset(100);
      
      op.restrictions = {rest};
      
      trx.operations.push_back(op);
      trx.validate();
      processed_transaction ptx = db.push_transaction(trx, ~0);
      trx.operations.clear();
      
      auto authorities = get_custom_authorities_by_account(db, dan.id);
      BOOST_REQUIRE_EQUAL(0, authorities.size());
   } catch (fc::exception &e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(create_custom_authority_operation_) {
   try {
      auto dan = create_account("dan");

      generate_block();
      fc::usleep(fc::milliseconds(2000));

      custom_authority_create_operation op;
      op.account = dan.id;
      op.enabled = true;
      op.valid_from = time_point_sec(1);
      op.valid_to = time_point_sec(2);
      op.operation_type = operation_type_id_from_operation_type<transfer_operation>::value;
      
      eq_restriction rest;
      rest.argument = "amount";
      rest.value = asset(100);
      
      op.restrictions = {rest};

      trx.operations.push_back(op);
      trx.validate();
      processed_transaction ptx = db.push_transaction(trx, ~0);
      trx.operations.clear();
      
      auto authorities = get_custom_authorities_by_account(db, dan.id);
      BOOST_REQUIRE_EQUAL(1, authorities.size());
      BOOST_CHECK(dan.id == authorities.front().account);
      BOOST_CHECK(authorities.front().enabled);
      BOOST_CHECK(time_point_sec(1) == authorities.front().valid_from);
      BOOST_CHECK(time_point_sec(2) == authorities.front().valid_to);
      BOOST_CHECK(operation_type_id_from_operation_type<transfer_operation>::value == authorities.front().operation_type.value);

      BOOST_REQUIRE_EQUAL(1, authorities.front().restrictions.size());
      
      auto restriction = authorities.front().restrictions.front().get<eq_restriction>();
      BOOST_CHECK_EQUAL("amount", restriction.argument);
      BOOST_CHECK(asset(100) == restriction.value.get<asset>());
      
   } catch (fc::exception &e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(delete_custom_authority) {
   try {
      auto dan = create_account("dan");
      
      generate_block();
      fc::usleep(fc::milliseconds(2000));
      
      custom_authority_create_operation op;
      op.account = dan.id;
      op.enabled = true;
      op.valid_from = time_point_sec(1);
      op.valid_to = time_point_sec(2);
      op.operation_type = operation_type_id_from_operation_type<transfer_operation>::value;
      
      eq_restriction rest;
      rest.argument = "amount";
      rest.value = asset(100);
      
      op.restrictions = {rest};
      
      trx.operations.push_back(op);
      trx.validate();
      processed_transaction ptx = db.push_transaction(trx, ~0);
      trx.operations.clear();
      
      auto authorities = get_custom_authorities_by_account(db, dan.id);
      BOOST_REQUIRE_EQUAL(1, authorities.size());
      
      {
         custom_authority_delete_operation op;
         op.custom_id = authorities.front().id;
         
         trx.operations.push_back(op);
         trx.validate();
         processed_transaction ptx = db.push_transaction(trx, ~0);
         trx.operations.clear();
      }
      
      authorities = get_custom_authorities_by_account(db, dan.id);
      BOOST_REQUIRE_EQUAL(0, authorities.size());
      
   } catch (fc::exception &e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_SUITE_END()
