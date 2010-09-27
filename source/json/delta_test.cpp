// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/unittest++.h"
#include "json/json.h"
#include "json/delta.h"

#include <iostream>

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