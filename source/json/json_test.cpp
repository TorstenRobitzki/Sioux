// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/unittest++.h"
#include "json/json.h"

TEST(json_string_test)
{
    CHECK_EQUAL(2u, json::string().size());
    CHECK_EQUAL("\"\"", json::string().to_json());

    const json::string s1("Hallo"), s2("Hallo");
    CHECK_EQUAL(s1, s2);
    CHECK_EQUAL(s1, s1);

    const json::string s3;

    CHECK(s1 != s3);
    CHECK_EQUAL("\"Hallo\"", s1.to_json());
    
    const json::string s4("\"\\\r");
    CHECK_EQUAL("\"\\\"\\\\\\r\"", s4.to_json());
}

TEST(json_number_test)
{
    json::number zweiundvierzig(42);
    CHECK_EQUAL("42", zweiundvierzig.to_json());
    CHECK_EQUAL(2u, zweiundvierzig.size());

    json::number null(0);
    CHECK_EQUAL("0", null.to_json());
    CHECK_EQUAL(1u, null.size());

    json::number negativ(-12);
    CHECK_EQUAL("-12", negativ.to_json());
    CHECK_EQUAL(3u, negativ.size());
}

TEST(json_object_test)
{
    const json::object empty;
    CHECK_EQUAL("{}", empty.to_json());
    CHECK_EQUAL(2u, empty.size());

    json::object obj;
    obj.add(json::string("Hallo"), json::number(123));
    json::object inner;
    
    obj.add(json::string("inner"), inner);
    inner.add(json::string("foo"), json::string("bar"));

    CHECK_EQUAL("{\"Hallo\":123,\"inner\":{\"foo\":\"bar\"}}", obj.to_json());
    CHECK_EQUAL(obj.to_json().size(), obj.size());

    CHECK_EQUAL("{\"foo\":\"bar\"}", inner.to_json());
    CHECK_EQUAL(inner.to_json().size(), inner.size());
}

TEST(json_array_test)
{
    json::array array;
    CHECK_EQUAL("[]", array.to_json());
    CHECK_EQUAL(2u, array.size());

    array.add(json::string("Hallo"));

    CHECK_EQUAL("[\"Hallo\"]", array.to_json());
    CHECK_EQUAL(array.to_json().size(), array.size());

    json::array inner;
    array.add(inner);

    inner.add(json::number(0));

    CHECK_EQUAL("[\"Hallo\",[0]]", array.to_json());
    CHECK_EQUAL(array.to_json().size(), array.size());
}

TEST(json_special_test)
{
    json::value null = json::null();
    CHECK_EQUAL(json::null(), null);
    CHECK_EQUAL("null", null.to_json());
    CHECK_EQUAL(null.to_json().size(), null.size());

    json::false_val f;
    CHECK_EQUAL("false", f.to_json());
    CHECK_EQUAL(f.to_json().size(), f.size());

    json::true_val t;
    CHECK_EQUAL("true", t.to_json());
    CHECK_EQUAL(t.to_json().size(), t.size());

    CHECK(t != f);
}
