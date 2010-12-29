// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/unittest++.h"
#include "pubsub/node_group.h"
#include "pubsub/key.h"
#include "pubsub/node.h"
#include "json/json.h"

/**
 * @test test the node_group compare operations
 */
TEST(compare_node_group_test)
{
    pubsub::node_group  default_group;
    pubsub::node_group  other_group(
        pubsub::build_node_group().
        has_domain(pubsub::key_domain("foofbar")));

    CHECK(default_group == pubsub::node_group(default_group));
    CHECK(pubsub::node_group(default_group) == default_group);
    CHECK(!(default_group != pubsub::node_group(default_group)));

    CHECK(other_group != default_group);
    CHECK(other_group == other_group);
    CHECK(other_group == pubsub::node_group(other_group));

    pubsub::node_group  even_other_group;
    even_other_group = other_group;

    CHECK(even_other_group == other_group);
    CHECK(even_other_group != default_group);
    CHECK(pubsub::node_group(even_other_group) == pubsub::node_group(other_group));
}

namespace {
    json::object parse(const char* text)
    {
        return json::parse(text).upcast<json::object>();
    }

    const pubsub::node_name   a_2_b_4(parse("{\"a\":2,\"b\":4}"));
    const pubsub::node_name   a_4_b_2(parse("{\"a\":4,\"b\":2}"));
    const pubsub::node_name   b_2_c_2(parse("{\"c\":2,\"b\":2}"));
    const pubsub::node_name   a_4_c_2(parse("{\"a\":4,\"c\":2}"));
}

/** 
 * @test check the domain filtering
 */
TEST(in_domain_node_group_test)
{  
    const pubsub::node_group  filter_all_a(
        has_domain(pubsub::key_domain("a")));

    const pubsub::node_group  filter_all_a_and_b(
        has_domain(pubsub::key_domain("a")).
        has_domain(pubsub::key_domain("b")));

    CHECK(filter_all_a.in_group(a_2_b_4));
    CHECK(filter_all_a.in_group(a_4_b_2));
    CHECK(filter_all_a.in_group(a_4_c_2));
    CHECK(!filter_all_a.in_group(pubsub::node_name()));
    CHECK(!filter_all_a.in_group(b_2_c_2));

    CHECK(filter_all_a_and_b.in_group(a_2_b_4));
    CHECK(filter_all_a_and_b.in_group(a_4_b_2));
    CHECK(!filter_all_a_and_b.in_group(a_4_c_2));
    CHECK(!filter_all_a_and_b.in_group(pubsub::node_name()));
    CHECK(!filter_all_a_and_b.in_group(b_2_c_2));
}

/** 
 * @test check the key filtering
 */
TEST(has_key_node_group_test)
{
    const pubsub::node_group filter_a_4(
        has_key(pubsub::key(pubsub::key_domain("a"), "4")));

    CHECK(!filter_a_4.in_group(a_2_b_4));
    CHECK(filter_a_4.in_group(a_4_b_2));
    CHECK(filter_a_4.in_group(a_4_c_2));
    CHECK(!filter_a_4.in_group(pubsub::node_name()));
    CHECK(!filter_a_4.in_group(b_2_c_2));

    const pubsub::node_group filter_a_4_c_2(
        has_key(pubsub::key(pubsub::key_domain("a"), "4")).
        has_key(pubsub::key(pubsub::key_domain("c"), "2")));

    CHECK(!filter_a_4_c_2.in_group(a_2_b_4));
    CHECK(!filter_a_4_c_2.in_group(a_4_b_2));
    CHECK(filter_a_4_c_2.in_group(a_4_c_2));
    CHECK(!filter_a_4_c_2.in_group(pubsub::node_name()));
    CHECK(!filter_a_4_c_2.in_group(b_2_c_2));
}

/** 
 * @test check the combined key and domain filtering
 */
TEST(has_key_has_domain_group_test)
{
    const pubsub::node_group filter_has_a_b_2(
        has_domain(pubsub::key_domain("a")).
        has_key(pubsub::key(pubsub::key_domain("b"), "2")));

    CHECK(!filter_has_a_b_2.in_group(a_2_b_4));
    CHECK(filter_has_a_b_2.in_group(a_4_b_2));
    CHECK(!filter_has_a_b_2.in_group(a_4_c_2));
    CHECK(!filter_has_a_b_2.in_group(pubsub::node_name()));
    CHECK(!filter_has_a_b_2.in_group(b_2_c_2));

    const pubsub::node_group filter_b_2_has_a(
        has_key(pubsub::key(pubsub::key_domain("b"), "2")).
        has_domain(pubsub::key_domain("a")));

    CHECK(!filter_b_2_has_a.in_group(a_2_b_4));
    CHECK(filter_b_2_has_a.in_group(a_4_b_2));
    CHECK(!filter_b_2_has_a.in_group(a_4_c_2));
    CHECK(!filter_b_2_has_a.in_group(pubsub::node_name()));
    CHECK(!filter_b_2_has_a.in_group(b_2_c_2));
}