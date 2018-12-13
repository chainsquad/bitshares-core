/*
 * Copyright (c) 2017 Cryptonomex, Inc., and contributors.
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

#include <graphene/chain/custom_authority_object.hpp>
#include <graphene/chain/protocol/restrictions.hpp>
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/custom_authorities_utils.hpp>

#include <iostream>

using namespace graphene::chain;

BOOST_AUTO_TEST_SUITE( custom_authority )

BOOST_AUTO_TEST_CASE( validation_for_correct_operation_name_is_passed )
{
   custom_authority_object auth;
   
   auth.operation_name = "graphene::chain::transfer_operation";
   BOOST_CHECK_NO_THROW(auth.validate(transfer_operation(), time_point_sec(0)));
   
   auth.operation_name = "graphene::chain::asset_create_operation";
   BOOST_CHECK_NO_THROW(auth.validate(asset_create_operation(), time_point_sec(0)));
}

BOOST_AUTO_TEST_CASE( validation_for_wrong_operation_name_is_failed )
{
   custom_authority_object auth;
   
   auth.operation_name = "graphene::chain::asset_create_operation";
   BOOST_CHECK_THROW(auth.validate(transfer_operation(), time_point_sec(0)), fc::exception);
   
   auth.operation_name = "graphene::chain::transfer_operation";
   BOOST_CHECK_THROW(auth.validate(asset_create_operation(), time_point_sec(0)), fc::exception);
}

BOOST_AUTO_TEST_CASE( validation_fails_when_now_is_after_valid_period )
{
   custom_authority_object auth;
   
   auth.operation_name = "graphene::chain::transfer_operation";
   auth.valid_from = time_point_sec(0);
   auth.valid_to = time_point_sec(5);
   BOOST_CHECK_THROW(auth.validate(transfer_operation(), time_point_sec(6)), fc::exception);
}

BOOST_AUTO_TEST_CASE( validation_fails_when_now_is_before_valid_period )
{
   graphene::chain::custom_authority_object auth;
   
   auth.operation_name = "graphene::chain::transfer_operation";
   auth.valid_from = time_point_sec(3);
   auth.valid_to = time_point_sec(5);
   BOOST_CHECK_THROW(auth.validate(transfer_operation(), time_point_sec(1)), fc::exception);
}

BOOST_AUTO_TEST_CASE( validation_passes_when_now_is_in_valid_period )
{
   custom_authority_object auth;
   
   auth.operation_name = "graphene::chain::transfer_operation";
   auth.valid_from = time_point_sec(3);
   auth.valid_to = time_point_sec(5);
   BOOST_CHECK_NO_THROW(auth.validate(transfer_operation(), time_point_sec(4)));
}

BOOST_AUTO_TEST_CASE( validation_passes_when_no_restrictions_for_operation_arguments )
{
   custom_authority_object auth;
   
   auth.operation_name = "graphene::chain::transfer_operation";
   auth.valid_from = time_point_sec(3);
   auth.valid_to = time_point_sec(5);
   
   auth.restrictions = {};
   
   BOOST_CHECK_NO_THROW(auth.validate(transfer_operation(), time_point_sec(4)));
}

BOOST_AUTO_TEST_CASE( validation_passes_when_one_restriction_passes_for_operation_arguments )
{
   custom_authority_object auth;
   
   auth.operation_name = "graphene::chain::transfer_operation";
   auth.valid_from = time_point_sec(3);
   auth.valid_to = time_point_sec(5);
   
   eq_restriction rest;
   rest.value = asset(5);
   rest.argument = "amount";
   
   auth.restrictions = {rest};
   
   transfer_operation op;
   op.amount = asset(5);
   
   BOOST_CHECK_NO_THROW(auth.validate(op, time_point_sec(4)));
}

BOOST_AUTO_TEST_CASE( validation_passes_when_several_restriction_passes_for_operation_arguments )
{
   custom_authority_object auth;
   
   auth.operation_name = "graphene::chain::transfer_operation";
   auth.valid_from = time_point_sec(3);
   auth.valid_to = time_point_sec(5);
   
   eq_restriction rest1;
   rest1.value = asset(5);
   rest1.argument = "amount";
   
   neq_restriction rest2;
   rest2.value = asset(6);
   rest2.argument = "amount";
   
   auth.restrictions = {rest1, rest2};
   
   transfer_operation op;
   op.amount = asset(5);
   
   BOOST_CHECK_NO_THROW(auth.validate(op, time_point_sec(4)));
}

BOOST_AUTO_TEST_CASE( validation_fails_when_one_restriction_fails_for_operation_arguments )
{
   custom_authority_object auth;
   
   auth.operation_name = "graphene::chain::transfer_operation";
   auth.valid_from = time_point_sec(3);
   auth.valid_to = time_point_sec(5);
   
   eq_restriction rest1;
   rest1.value = asset(5);
   rest1.argument = "amount";
   
   eq_restriction rest2;
   rest2.value = asset(6);
   rest2.argument = "amount";
   
   auth.restrictions = {rest1, rest2};
   
   transfer_operation op;
   op.amount = asset(5);
   
   BOOST_CHECK_THROW(auth.validate(op, time_point_sec(4)), fc::exception);
}

BOOST_AUTO_TEST_CASE( validate_eq_restriction_correctness_fails_when_argument_is_extensions_type )
{
   eq_restriction rest;
   rest.argument = "extensions";
   
   BOOST_CHECK_THROW(rest.validate<assert_operation>(), fc::exception);
}

BOOST_AUTO_TEST_CASE( validate_eq_restriction_correctness_passes_when_argument_is_asset )
{
   eq_restriction rest;
   rest.value = asset(5);
   rest.argument = "amount";
   
   BOOST_CHECK_NO_THROW(rest.validate<transfer_operation>());
}

BOOST_AUTO_TEST_CASE( validate_contains_all_restriction_correctness_passes_when_argument_is_list )
{
   contains_all_restriction rest;
   rest.argument = "required_auths";
   
   BOOST_CHECK_NO_THROW(rest.validate<assert_operation>());
}

BOOST_AUTO_TEST_CASE( validate_contains_all_restriction_correctness_fails_when_argument_is_not_list )
{
   contains_all_restriction rest;
   rest.argument = "amount";
   
   BOOST_CHECK_THROW(rest.validate<transfer_operation>(), fc::exception);
}

BOOST_AUTO_TEST_CASE( validate_contains_all_restriction_correctness_fails_when_argument_is_list_of_not_supported_values )
{
   contains_all_restriction rest;
   rest.argument = "predicates";
   
   BOOST_CHECK_THROW(rest.validate<assert_operation>(), fc::exception);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( custom_authority_restrictions )

BOOST_AUTO_TEST_CASE( validation_passes_for_eq_restriction_when_assets_are_equal )
{
   transfer_operation operation;
   operation.amount = asset(5);
   
   eq_restriction restriction;
   restriction.value = asset(5);
   restriction.argument = "amount";
   
   BOOST_CHECK_NO_THROW(restriction.validate(operation));
}

BOOST_AUTO_TEST_CASE( validation_fails_for_eq_restriction_when_assets_are_not_equal )
{
   transfer_operation operation;
   operation.amount = asset(5);
   
   eq_restriction restriction;
   restriction.value = asset(6);
   restriction.argument = "amount";
   
   BOOST_CHECK_THROW(restriction.validate(operation), fc::exception);
}

BOOST_AUTO_TEST_CASE( validation_fails_for_eq_restriction_when_comparing_asset_and_account )
{
   transfer_operation operation;
   operation.amount = asset(5);
   
   eq_restriction restriction;
   restriction.value = account_id_type(1);
   restriction.argument = "amount";
   
   BOOST_CHECK_THROW(restriction.validate(operation), fc::exception);
}

BOOST_AUTO_TEST_CASE( validation_passes_for_neq_restriction_when_assets_are_not_equal )
{
   transfer_operation operation;
   operation.amount = asset(5);
   
   neq_restriction restriction;
   restriction.value = asset(6);
   restriction.argument = "amount";
   
   BOOST_CHECK_NO_THROW(restriction.validate(operation));
}

BOOST_AUTO_TEST_CASE( validation_fails_for_neq_restriction_when_assets_are_equal )
{
   transfer_operation operation;
   operation.amount = asset(5);
   
   neq_restriction restriction;
   restriction.value = asset(5);
   restriction.argument = "amount";
   
   BOOST_CHECK_THROW(restriction.validate(operation), fc::exception);
}

BOOST_AUTO_TEST_CASE( validation_fails_for_neq_restriction_when_comparing_different_types )
{
   transfer_operation operation;
   operation.amount = asset(5);
   
   neq_restriction restriction;
   restriction.value = account_id_type(1);
   restriction.argument = "amount";
   
   BOOST_CHECK_THROW(restriction.validate(operation), fc::exception);
}

BOOST_AUTO_TEST_CASE( validation_passes_for_any_restriction_when_argument_value_is_present_in_the_list_with_single_value)
{
   transfer_operation operation;
   operation.amount = asset(5);
   
   any_restriction restriction;
   restriction.values = {asset(5)};
   restriction.argument = "amount";
   
   BOOST_CHECK_NO_THROW(restriction.validate(operation));
}

BOOST_AUTO_TEST_CASE( validation_passes_for_any_restriction_when_argument_value_is_present_in_the_list_with_several_values )
{
   transfer_operation operation;
   operation.amount = asset(5);
   
   any_restriction restriction;
   restriction.values = {asset(1), asset(2), asset(5)};
   restriction.argument = "amount";
   
   BOOST_CHECK_NO_THROW(restriction.validate(operation));
}

BOOST_AUTO_TEST_CASE( validation_fails_for_any_restriction_when_argument_value_is_not_present_in_the_list_with_several_values )
{
   transfer_operation operation;
   operation.amount = asset(5);
   
   any_restriction restriction;
   restriction.values = {asset(1), asset(2), asset(3)};
   restriction.argument = "amount";
   
   BOOST_CHECK_THROW(restriction.validate(operation), fc::exception);
}

BOOST_AUTO_TEST_CASE( validation_passes_for_none_restriction_when_argument_value_is_not_present_in_the_empty_list)
{
   transfer_operation operation;
   operation.amount = asset(4);
   
   none_restriction restriction;
   restriction.values = {};
   restriction.argument = "amount";
   
   BOOST_CHECK_NO_THROW(restriction.validate(operation));
}

BOOST_AUTO_TEST_CASE( validation_passes_for_none_restriction_when_argument_value_is_not_present_in_list)
{
   transfer_operation operation;
   operation.amount = asset(4);
   
   none_restriction restriction;
   restriction.values = {asset(1), asset(2)};
   restriction.argument = "amount";
   
   BOOST_CHECK_NO_THROW(restriction.validate(operation));
}

BOOST_AUTO_TEST_CASE( validation_fails_for_none_restriction_when_argument_value_is_present_in_list)
{
   transfer_operation operation;
   operation.amount = asset(2);
   
   none_restriction restriction;
   restriction.values = {asset(1), asset(2), asset(3)};
   restriction.argument = "amount";
   
   BOOST_CHECK_THROW(restriction.validate(operation), fc::exception);
}

BOOST_AUTO_TEST_CASE( validation_passes_for_conatins_all_restriction_when_argument_contains_list_values)
{
   assert_operation operation;
   operation.required_auths = {account_id_type(1), account_id_type(2), account_id_type(3)};
   
   contains_all_restriction restriction;
   restriction.values = {account_id_type(1), account_id_type(2), account_id_type(3)};
   restriction.argument = "required_auths";
   
   BOOST_CHECK_NO_THROW(restriction.validate(operation));
}

BOOST_AUTO_TEST_CASE( validation_failes_for_conatins_all_restriction_when_argument_contains_subset_of_list_values)
{
   assert_operation operation;
   operation.required_auths = {account_id_type(1), account_id_type(2), account_id_type(3)};
   
   contains_all_restriction restriction;
   restriction.values = {account_id_type(0), account_id_type(1), account_id_type(2), account_id_type(3), account_id_type(4)};
   restriction.argument = "required_auths";
   
   BOOST_CHECK_THROW(restriction.validate(operation), fc::exception);
}

BOOST_AUTO_TEST_CASE( validation_passes_for_conatins_all_restriction_when_argument_contains_superset_of_list_values)
{
   assert_operation operation;
   operation.required_auths = {account_id_type(0), account_id_type(1), account_id_type(2), account_id_type(3), account_id_type(4)};
   
   contains_all_restriction restriction;
   restriction.values = {account_id_type(1), account_id_type(2), account_id_type(3)};
   restriction.argument = "required_auths";
   
   BOOST_CHECK_NO_THROW(restriction.validate(operation));
}

BOOST_AUTO_TEST_CASE( validation_passes_for_contains_none_restriction_when_argument_not_contains_any_of_list_values)
{
   assert_operation operation;
   operation.required_auths = {account_id_type(0), account_id_type(1), account_id_type(2)};
   
   contains_none_restriction restriction;
   restriction.values = {account_id_type(3), account_id_type(4)};
   restriction.argument = "required_auths";
   
   BOOST_CHECK_NO_THROW(restriction.validate(operation));
}

BOOST_AUTO_TEST_CASE( validation_fails_for_contains_none_restriction_when_argument_contained_any_of_list_values)
{
   assert_operation operation;
   operation.required_auths = {account_id_type(0), account_id_type(1), account_id_type(2)};
   
   contains_none_restriction restriction;
   restriction.values = {account_id_type(1)};
   restriction.argument = "required_auths";
   
   BOOST_CHECK_THROW(restriction.validate(operation), fc::exception);
}

BOOST_AUTO_TEST_CASE( validation_fails_for_contains_none_restriction_when_argument_contained_several_of_list_values)
{
   assert_operation operation;
   operation.required_auths = {account_id_type(0), account_id_type(1), account_id_type(2)};
   
   contains_none_restriction restriction;
   restriction.values = {account_id_type(1), account_id_type(2)};
   restriction.argument = "required_auths";
   
   BOOST_CHECK_THROW(restriction.validate(operation), fc::exception);
}

BOOST_AUTO_TEST_CASE( lt_restriction_passes_for_argument_less_than_value)
{
   account_create_operation operation;
   operation.referrer_percent = 50;
   
   lt_restriction restriction;
   restriction.value = 60;
   restriction.argument = "referrer_percent";
   
   BOOST_CHECK_NO_THROW(restriction.validate(operation));
}

BOOST_AUTO_TEST_CASE( lt_restriction_fails_for_argument_equals_to_value)
{
   account_create_operation operation;
   operation.referrer_percent = 50;
   
   lt_restriction restriction;
   restriction.value = 50;
   restriction.argument = "referrer_percent";
   
   BOOST_CHECK_THROW(restriction.validate(operation), fc::exception);
}

BOOST_AUTO_TEST_CASE( lt_restriction_fails_for_argument_greater_than_value)
{
   account_create_operation operation;
   operation.referrer_percent = 60;
   
   lt_restriction restriction;
   restriction.value = 50;
   restriction.argument = "referrer_percent";
   
   BOOST_CHECK_THROW(restriction.validate(operation), fc::exception);
}

BOOST_AUTO_TEST_CASE( le_restriction_passes_for_argument_less_than_value)
{
   account_create_operation operation;
   operation.referrer_percent = 50;
   
   le_restriction restriction;
   restriction.value = 60;
   restriction.argument = "referrer_percent";
   
   BOOST_CHECK_NO_THROW(restriction.validate(operation));
}

BOOST_AUTO_TEST_CASE( le_restriction_passes_for_argument_equals_to_value)
{
   account_create_operation operation;
   operation.referrer_percent = 50;
   
   le_restriction restriction;
   restriction.value = 50;
   restriction.argument = "referrer_percent";
   
   BOOST_CHECK_NO_THROW(restriction.validate(operation));
}

BOOST_AUTO_TEST_CASE( le_restriction_fails_for_argument_greater_than_value)
{
   account_create_operation operation;
   operation.referrer_percent = 60;
   
   le_restriction restriction;
   restriction.value = 50;
   restriction.argument = "referrer_percent";
   
   BOOST_CHECK_THROW(restriction.validate(operation), fc::exception);
}

BOOST_AUTO_TEST_CASE( gt_restriction_fails_for_argument_less_than_value)
{
   account_create_operation operation;
   operation.referrer_percent = 50;
   
   gt_restriction restriction;
   restriction.value = 60;
   restriction.argument = "referrer_percent";
   
   BOOST_CHECK_THROW(restriction.validate(operation), fc::exception);
}

BOOST_AUTO_TEST_CASE( gt_restriction_fails_for_argument_equals_to_value)
{
   account_create_operation operation;
   operation.referrer_percent = 50;
   
   gt_restriction restriction;
   restriction.value = 50;
   restriction.argument = "referrer_percent";
   
   BOOST_CHECK_THROW(restriction.validate(operation), fc::exception);
}

BOOST_AUTO_TEST_CASE( gt_restriction_passes_for_argument_greater_than_value)
{
   account_create_operation operation;
   operation.referrer_percent = 60;
   
   gt_restriction restriction;
   restriction.value = 50;
   restriction.argument = "referrer_percent";
   
   BOOST_CHECK_NO_THROW(restriction.validate(operation));
}

BOOST_AUTO_TEST_CASE( ge_restriction_fails_for_argument_less_than_value)
{
   account_create_operation operation;
   operation.referrer_percent = 50;
   
   ge_restriction restriction;
   restriction.value = 60;
   restriction.argument = "referrer_percent";
   
   BOOST_CHECK_THROW(restriction.validate(operation), fc::exception);
}

BOOST_AUTO_TEST_CASE( ge_restriction_passes_for_argument_equals_to_value)
{
   account_create_operation operation;
   operation.referrer_percent = 50;
   
   ge_restriction restriction;
   restriction.value = 50;
   restriction.argument = "referrer_percent";
   
   BOOST_CHECK_NO_THROW(restriction.validate(operation));
}

BOOST_AUTO_TEST_CASE( ge_restriction_passes_for_argument_greater_than_value)
{
   account_create_operation operation;
   operation.referrer_percent = 60;
   
   ge_restriction restriction;
   restriction.value = 50;
   restriction.argument = "referrer_percent";
   
   BOOST_CHECK_NO_THROW(restriction.validate(operation));
}

BOOST_AUTO_TEST_SUITE_END()
