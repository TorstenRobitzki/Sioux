// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/UnitTest++.h"
#include "json/json.h"
#include "json/delta.h"
#include "tools/asstring.h"
#include <limits>

TEST(array_update)
{
    const json::value start    = json::parse("[1,2,null,\"gone\"]");
    const json::value update   = json::parse("[1,1,{},2,3,3,0,42]");
    const json::value expected = json::parse("[42,1,{},null]");

    CHECK_EQUAL(expected, json::update(start, update));
    CHECK_EQUAL(json::null(), json::update(start, json::null()));
    CHECK_EQUAL(json::number(1), json::update(update, json::number(1)));
}

TEST(array_range_operations)
{
    const json::value start    = json::array();
    const json::value update   = json::parse("[5,0,0,[1,2,3,4],4,1,3,3,2,\"end\",5,1,1,[]]");
    const json::value expected = json::parse("[1,4,\"end\"]");

    CHECK_EQUAL(expected, json::update(start, update));
    CHECK_EQUAL(start, json::update(start, json::array()));
}

TEST(array_range_operations2)
{
    const json::value start    = json::parse("[1,2,3]");
    const json::value update   = json::parse("[5,0,3,[4,5,6]]");
    const json::value expected = json::parse("[4,5,6]");

    CHECK_EQUAL(expected, json::update(start, update));
    CHECK_EQUAL(start, json::update(start, json::array()));
}

TEST(object_update)
{
    const json::value start    = json::parse("{\"foo\":null, \"bar\" : 1, \"ar\": []}");
    const json::value update   = json::parse("[1,\"foo\",false,2,\"ar\",3,\"nase\",{}]");
    const json::value expected = json::parse("{\"foo\":false, \"bar\" : 1, \"nase\": {}}");

    CHECK_EQUAL(expected, json::update(start, update));
    CHECK_EQUAL(json::null(), json::update(start, json::null()));
    CHECK_EQUAL(json::number(1), json::update(update, json::number(1)));
}

TEST(edit_operations)
{
    const json::value   start_array  = json::parse("[1,2,[4,5]]");
    const json::value   update_array = json::parse("[6,0,null,6,2,[3,0,3]]");

    CHECK_EQUAL(
        json::parse("[null,2,[3,4,5]]"),
        json::update(start_array, update_array));

    const json::value   start_obj  = json::parse("{\"foo\":[1,2,3,4], \"bar\":{}}");
    const json::value   update_obj = json::parse("[6,\"bar\",null,6,\"foo\",[4,1,3]]");

    CHECK_EQUAL(
        json::parse("{\"foo\":[1,4], \"bar\":null}"),
        json::update(start_obj, update_obj));

}
namespace 
{
    bool apply_delta_and_update(const json::value& a, const json::value& b)
    {
        const std::pair<bool, json::value> delta = json::delta(a, b, std::numeric_limits<std::size_t>::max());
        return delta.first && update(a, delta.second) == b;
    }

    bool apply_delta_and_update(const std::string& a, const std::string& b)
    {
        return apply_delta_and_update(json::parse(a), json::parse(b));
    }

    std::string delta(const std::string& a, const std::string& b)
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
TEST(update_array_will_result_in_unchanged_paramter)
{
	const char* const original_text = "[1,2,3]";
	const json::value original = json::parse(original_text);
	CHECK_EQUAL(json::parse("[4,5,6]"), update(original, json::parse("[5,0,3,[4,5,6]]")));
	CHECK_EQUAL(tools::as_string(original), original_text);
	CHECK_EQUAL(original, json::parse(original_text));
}

TEST(simple_delta)
{
    // single insert, delete and update 
    CHECK(apply_delta_and_update("[]", "[1]"));
    CHECK(apply_delta_and_update("[1]", "[]"));
    CHECK(apply_delta_and_update("[1]", "[2]"));

    // check that single delete and converting to range delete works
    CHECK_EQUAL("[2,0]", delta("[1,2,3]", "[2,3]"));
    CHECK_EQUAL("[2,1]", delta("[1,2,3]", "[1,3]"));
    CHECK_EQUAL("[2,2]", delta("[1,2,3]", "[1,2]"));
    CHECK_EQUAL("[2,1,4,2,5]", delta("[1,2,3,4,5,6]", "[1,3]"));
    CHECK_EQUAL("[4,0,6]", delta("[1,2,3,4,5,6]", "[]"));
    CHECK_EQUAL("[4,2,4]", delta("[1,2,3,4,5,6]", "[1,2,5,6]"));
    CHECK_EQUAL("[4,2,4]", delta("[1,2,3,4]", "[1,2]"));
    CHECK_EQUAL("[4,0,2]", delta("[1,2,3,4]", "[3,4]"));

    // single updates and converting to range updates 
    CHECK_EQUAL("[1,0,9]", delta("[1,2,3]", "[9,2,3]"));
    CHECK_EQUAL("[1,1,9]", delta("[1,2,3]", "[1,9,3]"));
    CHECK_EQUAL("[1,2,9]", delta("[1,2,3]", "[1,2,9]"));
    CHECK_EQUAL("[5,0,3,[4,5,6]]", delta("[1,2,3]", "[4,5,6]"));
    CHECK_EQUAL("[5,0,3,[4,5]]", delta("[1,2,3]", "[4,5]"));
    CHECK_EQUAL("[5,0,4,[9]]", delta("[1,2,3,4,5,6,7]", "[9,5,6,7]"));
    CHECK_EQUAL("[5,3,7,[9]]", delta("[1,2,3,4,5,6,7]", "[1,2,3,9]"));
    CHECK_EQUAL("[5,2,5,[9]]", delta("[1,2,3,4,5,6,7]", "[1,2,9,6,7]"));
    CHECK_EQUAL("[5,1,2,[5,5]]", delta("[1,2]", "[1,5,5]"));
    CHECK_EQUAL("[5,3,7,[9,9,9,9]]", delta("[1,2,3,4,5,6,7]", "[1,2,3,9,9,9,9]"));
    CHECK_EQUAL("[5,3,5,[9,9,9,9]]", delta("[1,2,3,4,5,6,7]", "[1,2,3,9,9,9,9,6,7]"));
    CHECK_EQUAL("[5,1,6,[9]]", delta("[1,2,3,4,5,6,7]", "[1,9,7]"));
    CHECK_EQUAL("[5,1,6,[20,9,20]]", delta("[1,2,3,4,5,6,7]", "[1,20,9,20,7]"));

    // in some cases, repeating elements in an update array may be shorter then two updates
    CHECK_EQUAL("[5,2,5,[9,9,4,9,9]]", delta("[1,2,3,4,5,6,7]", "[1,2,9,9,4,9,9,6,7]"));
}

TEST(equal_test)
{
    CHECK_EQUAL("[]", delta("[1,2,{},false]", "[1,2,{},false]"));
}

TEST(edit_test)
{
    CHECK_EQUAL("[6,0,[2,0]]", delta("[[1,2,3,4,5,6,7]]", "[[2,3,4,5,6,7]]"));
    CHECK_EQUAL("[1,0,[6,7]]", delta("[[1,2,3,4,5,6,7]]", "[[6,7]]"));
    CHECK_EQUAL("[6,0,[6,1,[2,0]]]", delta("[[1,[1,2,3,4,5,6,7]]]", "[[1,[2,3,4,5,6,7]]]"));
}

TEST(object_delta)
{
    CHECK_EQUAL("[1,\"A\",2]", delta("{\"A\":1}", "{\"A\":2}"));
    CHECK_EQUAL("[2,\"A\"]", delta("{\"A\":1}", "{}"));
    CHECK_EQUAL("[3,\"A\",2]", delta("{}", "{\"A\":2}"));
    CHECK_EQUAL("[1,\"A\",[2,3]]", delta("{\"A\":[1,2,3]}", "{\"A\":[2,3]}"));
    CHECK_EQUAL("[6,\"A\",[2,0]]", delta("{\"A\":[1,2,3,4,5,6,7,8,9,1,2,3,4]}", "{\"A\":[2,3,4,5,6,7,8,9,1,2,3,4]}"));
    CHECK_EQUAL("[2,\"A\",3,\"B\",1]", delta("{\"A\":1}", "{\"B\":1}"));
    CHECK_EQUAL("[2,\"A\",1,\"B\",2,2,\"C\",3,\"D\",1]", delta("{\"A\":1,\"B\":1,\"C\":1}", "{\"B\":2,\"D\":1}"));
}
