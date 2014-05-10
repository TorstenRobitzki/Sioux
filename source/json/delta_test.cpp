// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>
#include <boost/timer/timer.hpp>
#include "json/json.h"
#include "json/delta.h"
#include "tools/asstring.h"
#include <limits>
#include <fstream>

BOOST_AUTO_TEST_CASE(array_update)
{
    const json::value start    = json::parse("[1,2,null,\"gone\"]");
    const json::value update   = json::parse("[1,1,{},2,3,3,0,42]");
    const json::value expected = json::parse("[42,1,{},null]");

    BOOST_CHECK_EQUAL(expected, json::update(start, update));
    BOOST_CHECK_EQUAL(json::null(), json::update(start, json::null()));
    BOOST_CHECK_EQUAL(json::number(1), json::update(update, json::number(1)));
}

BOOST_AUTO_TEST_CASE(array_range_operations)
{
    const json::value start    = json::array();
    const json::value update   = json::parse("[5,0,0,[1,2,3,4],4,1,3,3,2,\"end\",5,1,1,[]]");
    const json::value expected = json::parse("[1,4,\"end\"]");

    BOOST_CHECK_EQUAL(expected, json::update(start, update));
    BOOST_CHECK_EQUAL(start, json::update(start, json::array()));
}

BOOST_AUTO_TEST_CASE(array_range_operations2)
{
    const json::value start    = json::parse("[1,2,3]");
    const json::value update   = json::parse("[5,0,3,[4,5,6]]");
    const json::value expected = json::parse("[4,5,6]");

    BOOST_CHECK_EQUAL(expected, json::update(start, update));
    BOOST_CHECK_EQUAL(start, json::update(start, json::array()));
}

BOOST_AUTO_TEST_CASE(object_update)
{
    const json::value start    = json::parse("{\"foo\":null, \"bar\" : 1, \"ar\": []}");
    const json::value update   = json::parse("[1,\"foo\",false,2,\"ar\",3,\"nase\",{}]");
    const json::value expected = json::parse("{\"foo\":false, \"bar\" : 1, \"nase\": {}}");

    BOOST_CHECK_EQUAL(expected, json::update(start, update));
    BOOST_CHECK_EQUAL(json::null(), json::update(start, json::null()));
    BOOST_CHECK_EQUAL(json::number(1), json::update(update, json::number(1)));
}

BOOST_AUTO_TEST_CASE(edit_operations)
{
    const json::value   start_array  = json::parse("[1,2,[4,5]]");
    const json::value   update_array = json::parse("[6,0,null,6,2,[3,0,3]]");

    BOOST_CHECK_EQUAL(
        json::parse("[null,2,[3,4,5]]"),
        json::update(start_array, update_array));

    const json::value   start_obj  = json::parse("{\"foo\":[1,2,3,4], \"bar\":{}}");
    const json::value   update_obj = json::parse("[6,\"bar\",null,6,\"foo\",[4,1,3]]");

    BOOST_CHECK_EQUAL(
        json::parse("{\"foo\":[1,4], \"bar\":null}"),
        json::update(start_obj, update_obj));

}
namespace
{
    void check_delta_and_update(const json::value& a, const json::value& b)
    {
        const std::pair<bool, json::value> delta = json::delta(a, b, std::numeric_limits<std::size_t>::max());
        BOOST_REQUIRE( delta.first );
        BOOST_CHECK_EQUAL( update(a, delta.second), b );
    }

    void check_delta_and_update(const std::string& a, const std::string& b)
    {
        return check_delta_and_update(json::parse(a), json::parse(b));
    }

    std::string delta( const std::string& a, const std::string& b )
    {
        const json::value av = json::parse(a);
        const json::value bv = json::parse(b);

        const std::pair<bool, json::value> delta = json::delta(av, bv, std::numeric_limits<std::size_t>::max());

        if ( delta.first && update(av, delta.second) != bv )
        	throw std::runtime_error("update(\"" + tools::as_string(av) +"\",\"" + tools::as_string(delta.second) +
                " != \"" + tools::as_string(bv) + "\"");

        return delta.first ? tools::as_string(delta.second) : "";
    }
}

/**
 * @test while porting to gcc a bug, changing the input parameter popped up
 */
BOOST_AUTO_TEST_CASE(update_array_will_result_in_unchanged_paramter)
{
	const char* const original_text = "[1,2,3]";
	const json::value original = json::parse(original_text);
	BOOST_CHECK_EQUAL(json::parse("[4,5,6]"), update(original, json::parse("[5,0,3,[4,5,6]]")));
	BOOST_CHECK_EQUAL(tools::as_string(original), original_text);
	BOOST_CHECK_EQUAL(original, json::parse(original_text));
}

BOOST_AUTO_TEST_CASE( single_insert )
{
    check_delta_and_update( "[]", "[1]" );
}

BOOST_AUTO_TEST_CASE( single_delete )
{
    check_delta_and_update( "[1]", "[]" );
}

BOOST_AUTO_TEST_CASE( single_update )
{
    check_delta_and_update( "[1]", "[2]" );
}

BOOST_AUTO_TEST_CASE( single_delete_and_conversion_to_range_delete )
{
    BOOST_CHECK_EQUAL("[2,0]", delta("[1,2,3]", "[2,3]"));
    BOOST_CHECK_EQUAL("[2,1]", delta("[1,2,3]", "[1,3]"));
    BOOST_CHECK_EQUAL("[2,2]", delta("[1,2,3]", "[1,2]"));
    BOOST_CHECK_EQUAL("[2,1,4,2,5]", delta("[1,2,3,4,5,6]", "[1,3]"));
    BOOST_CHECK_EQUAL("[4,0,6]", delta("[1,2,3,4,5,6]", "[]"));
    BOOST_CHECK_EQUAL("[4,2,4]", delta("[1,2,3,4,5,6]", "[1,2,5,6]"));
    BOOST_CHECK_EQUAL("[4,2,4]", delta("[1,2,3,4]", "[1,2]"));
    BOOST_CHECK_EQUAL("[4,0,2]", delta("[1,2,3,4]", "[3,4]"));
}

BOOST_AUTO_TEST_CASE( single_updates_and_convertion_to_range_updates )
{
    BOOST_CHECK_EQUAL("[1,0,9]", delta("[1,2,3]", "[9,2,3]"));
    BOOST_CHECK_EQUAL("[1,1,9]", delta("[1,2,3]", "[1,9,3]"));
    BOOST_CHECK_EQUAL("[1,2,9]", delta("[1,2,3]", "[1,2,9]"));
    BOOST_CHECK_EQUAL("[5,0,3,[4,5,6]]", delta("[1,2,3]", "[4,5,6]"));
    BOOST_CHECK_EQUAL("[5,0,3,[4,5]]", delta("[1,2,3]", "[4,5]"));
    BOOST_CHECK_EQUAL("[5,0,4,[9]]", delta("[1,2,3,4,5,6,7]", "[9,5,6,7]"));
    BOOST_CHECK_EQUAL("[5,3,7,[9]]", delta("[1,2,3,4,5,6,7]", "[1,2,3,9]"));
    BOOST_CHECK_EQUAL("[5,2,5,[9]]", delta("[1,2,3,4,5,6,7]", "[1,2,9,6,7]"));
    BOOST_CHECK_EQUAL("[5,1,2,[5,5]]", delta("[1,2]", "[1,5,5]"));
    BOOST_CHECK_EQUAL("[5,3,7,[9,9,9,9]]", delta("[1,2,3,4,5,6,7]", "[1,2,3,9,9,9,9]"));
    BOOST_CHECK_EQUAL("[5,3,5,[9,9,9,9]]", delta("[1,2,3,4,5,6,7]", "[1,2,3,9,9,9,9,6,7]"));
    BOOST_CHECK_EQUAL("[5,1,6,[9]]", delta("[1,2,3,4,5,6,7]", "[1,9,7]"));
    BOOST_CHECK_EQUAL("[5,1,6,[20,9,20]]", delta("[1,2,3,4,5,6,7]", "[1,20,9,20,7]"));
}

BOOST_AUTO_TEST_CASE( repeating_elements_in_an_update_may_be_shorter_then_two_updates )
{
    BOOST_CHECK_EQUAL("[5,2,5,[9,9,4,9,9]]", delta("[1,2,3,4,5,6,7]", "[1,2,9,9,4,9,9,6,7]"));
}

BOOST_AUTO_TEST_CASE(equal_test)
{
    BOOST_CHECK_EQUAL("[]", delta("[1,2,{},false]", "[1,2,{},false]"));
}

BOOST_AUTO_TEST_CASE(edit_test)
{
    BOOST_CHECK_EQUAL("[6,0,[2,0]]", delta("[[1,2,3,4,5,6,7]]", "[[2,3,4,5,6,7]]"));
    BOOST_CHECK_EQUAL("[1,0,[6,7]]", delta("[[1,2,3,4,5,6,7]]", "[[6,7]]"));
    BOOST_CHECK_EQUAL("[6,0,[6,1,[2,0]]]", delta("[[1,[1,2,3,4,5,6,7]]]", "[[1,[2,3,4,5,6,7]]]"));
}

BOOST_AUTO_TEST_CASE(object_delta)
{
    BOOST_CHECK_EQUAL("[1,\"A\",2]", delta("{\"A\":1}", "{\"A\":2}"));
    BOOST_CHECK_EQUAL("[2,\"A\"]", delta("{\"A\":1}", "{}"));
    BOOST_CHECK_EQUAL("[3,\"A\",2]", delta("{}", "{\"A\":2}"));
    BOOST_CHECK_EQUAL("[1,\"A\",[2,3]]", delta("{\"A\":[1,2,3]}", "{\"A\":[2,3]}"));
    BOOST_CHECK_EQUAL("[6,\"A\",[2,0]]", delta("{\"A\":[1,2,3,4,5,6,7,8,9,1,2,3,4]}", "{\"A\":[2,3,4,5,6,7,8,9,1,2,3,4]}"));
    BOOST_CHECK_EQUAL("[2,\"A\",3,\"B\",1]", delta("{\"A\":1}", "{\"B\":1}"));
    BOOST_CHECK_EQUAL("[2,\"A\",1,\"B\",2,2,\"C\",3,\"D\",1]", delta("{\"A\":1,\"B\":1,\"C\":1}", "{\"B\":2,\"D\":1}"));
}

/* this test tries to reproduce some endless loop found in the wild */
BOOST_AUTO_TEST_CASE( delta_of_larger_structure )
{
    std::ifstream input( "./source/json/fixtures.json" );
    std::istreambuf_iterator< char > begin( input ), end;

    json::array fixture = json::parse( begin, end ).upcast< json::array >();
    json::array a = fixture.at( 0 ).upcast< json::array >();
    json::array b = fixture.at( 1 ).upcast< json::array >();

    check_delta_and_update( a, b );
}
