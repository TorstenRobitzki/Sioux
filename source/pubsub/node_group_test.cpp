// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include <boost/test/unit_test.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>
#include "pubsub/node_group.h"
#include "pubsub/key.h"
#include "pubsub/node.h"
#include "json/json.h"
#include "tools/asstring.h"

/**
 * @test test the node_group compare operations
 */
BOOST_AUTO_TEST_CASE(compare_node_group_test)
{
    pubsub::node_group  default_group;
    pubsub::node_group  other_group(
        pubsub::build_node_group().
        has_domain(pubsub::key_domain("foofbar")));

    BOOST_CHECK(default_group == pubsub::node_group(default_group));
    BOOST_CHECK(pubsub::node_group(default_group) == default_group);
    BOOST_CHECK(!(default_group != pubsub::node_group(default_group)));

    BOOST_CHECK(other_group != default_group);
    BOOST_CHECK(other_group == other_group);
    BOOST_CHECK(other_group == pubsub::node_group(other_group));

    pubsub::node_group  even_other_group;
    even_other_group = other_group;

    BOOST_CHECK(even_other_group == other_group);
    BOOST_CHECK(even_other_group != default_group);
    BOOST_CHECK(pubsub::node_group(even_other_group) == pubsub::node_group(other_group));
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
BOOST_AUTO_TEST_CASE(in_domain_node_group_test)
{  
    const pubsub::node_group  filter_all_a(
        has_domain(pubsub::key_domain("a")));

    const pubsub::node_group  filter_all_a_and_b(
        has_domain(pubsub::key_domain("a")).
        has_domain(pubsub::key_domain("b")));

    BOOST_CHECK(filter_all_a.in_group(a_2_b_4));
    BOOST_CHECK(filter_all_a.in_group(a_4_b_2));
    BOOST_CHECK(filter_all_a.in_group(a_4_c_2));
    BOOST_CHECK(!filter_all_a.in_group(pubsub::node_name()));
    BOOST_CHECK(!filter_all_a.in_group(b_2_c_2));

    BOOST_CHECK(filter_all_a_and_b.in_group(a_2_b_4));
    BOOST_CHECK(filter_all_a_and_b.in_group(a_4_b_2));
    BOOST_CHECK(!filter_all_a_and_b.in_group(a_4_c_2));
    BOOST_CHECK(!filter_all_a_and_b.in_group(pubsub::node_name()));
    BOOST_CHECK(!filter_all_a_and_b.in_group(b_2_c_2));
}

/** 
 * @test check the key filtering
 */
BOOST_AUTO_TEST_CASE(has_key_node_group_test)
{
    const pubsub::node_group filter_a_4(
        has_key(pubsub::key(pubsub::key_domain("a"), "4")));

    BOOST_CHECK(!filter_a_4.in_group(a_2_b_4));
    BOOST_CHECK(filter_a_4.in_group(a_4_b_2));
    BOOST_CHECK(filter_a_4.in_group(a_4_c_2));
    BOOST_CHECK(!filter_a_4.in_group(pubsub::node_name()));
    BOOST_CHECK(!filter_a_4.in_group(b_2_c_2));

    const pubsub::node_group filter_a_4_c_2(
        has_key(pubsub::key(pubsub::key_domain("a"), "4")).
        has_key(pubsub::key(pubsub::key_domain("c"), "2")));

    BOOST_CHECK(!filter_a_4_c_2.in_group(a_2_b_4));
    BOOST_CHECK(!filter_a_4_c_2.in_group(a_4_b_2));
    BOOST_CHECK(filter_a_4_c_2.in_group(a_4_c_2));
    BOOST_CHECK(!filter_a_4_c_2.in_group(pubsub::node_name()));
    BOOST_CHECK(!filter_a_4_c_2.in_group(b_2_c_2));
}

/** 
 * @test check the combined key and domain filtering
 */
BOOST_AUTO_TEST_CASE(has_key_has_domain_group_test)
{
    const pubsub::node_group filter_has_a_b_2(
        has_domain(pubsub::key_domain("a")).
        has_key(pubsub::key(pubsub::key_domain("b"), "2")));

    BOOST_CHECK(!filter_has_a_b_2.in_group(a_2_b_4));
    BOOST_CHECK(filter_has_a_b_2.in_group(a_4_b_2));
    BOOST_CHECK(!filter_has_a_b_2.in_group(a_4_c_2));
    BOOST_CHECK(!filter_has_a_b_2.in_group(pubsub::node_name()));
    BOOST_CHECK(!filter_has_a_b_2.in_group(b_2_c_2));

    const pubsub::node_group filter_b_2_has_a(
        has_key(pubsub::key(pubsub::key_domain("b"), "2")).
        has_domain(pubsub::key_domain("a")));

    BOOST_CHECK(!filter_b_2_has_a.in_group(a_2_b_4));
    BOOST_CHECK(filter_b_2_has_a.in_group(a_4_b_2));
    BOOST_CHECK(!filter_b_2_has_a.in_group(a_4_c_2));
    BOOST_CHECK(!filter_b_2_has_a.in_group(pubsub::node_name()));
    BOOST_CHECK(!filter_b_2_has_a.in_group(b_2_c_2));
}

/**
 * @test equaly_distributed_node_groups returns a list of the given size
 */
BOOST_AUTO_TEST_CASE( equaly_distributed_node_groups_returns_a_list_of_the_given_size )
{
    BOOST_CHECK_EQUAL( pubsub::equaly_distributed_node_groups( 4u ).size(), 4u );
    BOOST_CHECK_EQUAL( pubsub::equaly_distributed_node_groups( 1u ).size(), 1u );
    BOOST_CHECK_EQUAL( pubsub::equaly_distributed_node_groups( 14u ).size(), 14u );
    BOOST_CHECK_EQUAL( pubsub::equaly_distributed_node_groups( 7u ).size(), 7u );
}

std::vector< pubsub::node_name > generate_random_names( unsigned n )
{
    boost::minstd_rand random;

    typedef boost::uniform_int< int > distribution_type;
    typedef boost::variate_generator< boost::minstd_rand&, distribution_type > gen_type;

    distribution_type distribution(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    gen_type die_gen(random, distribution);

    std::vector< pubsub::node_name > names( n );
    for ( std::vector< pubsub::node_name >::iterator name = names.begin(); name != names.end(); ++name )
    {
        name->add( pubsub::key( pubsub::key_domain( "A" ), tools::as_string( die_gen() ) ) );
    }

    return names;
}

/**
 * @test the result of equaly_distributed_node_groups distributes keys with 3 different domains
 *       over 3 different key_groups
 */
BOOST_AUTO_TEST_CASE( equaly_distributed_node_groups_distributes_equaly_over_3_groups )
{
    unsigned const number_of_groups = 3u;
    unsigned const number_of_names  = 1000u;

    const std::vector< pubsub::node_name > names = generate_random_names( number_of_names );
    const std::vector< pubsub::node_group > groups = pubsub::equaly_distributed_node_groups( number_of_groups );
    std::vector< unsigned > count( number_of_groups, 0u );

    for ( std::vector< pubsub::node_name >::const_iterator name = names.begin(); name != names.end(); ++name )
    {
        bool found = false;
        for ( unsigned group_idx = 0; group_idx != number_of_groups; ++group_idx )
        {
            if ( found )
            {
                BOOST_REQUIRE( !groups[ group_idx ].in_group( *name ) );
            }
            else if ( groups[ group_idx ].in_group( *name ) )
            {
                found = true;
                ++count[ group_idx ];
            }
        }

        BOOST_REQUIRE( found );
    }

    for ( unsigned group_idx = 0; group_idx != number_of_groups; ++group_idx )
        BOOST_CHECK_CLOSE( static_cast< double >( number_of_names ) / number_of_groups,
            static_cast< double >( count[ group_idx ] ), 0.01 );
}
