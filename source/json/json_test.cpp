// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/unittest++.h"
#include "json/json.h"
#include "tools/iterators.h"
#include <iostream>

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

TEST(json_array_copy_at_begin_test)
{
    const json::array array = json::parse("[1,2,3,4,5,6,7]").upcast<json::array>();

    CHECK_EQUAL("[1,2,3,4,5,6,7]", array.to_json());
    CHECK_EQUAL("[1,2,3,4,5,6,7]", json::array(array,7u).to_json());
    CHECK_EQUAL("[1,2,3,4]", json::array(array,4u).to_json());
    CHECK_EQUAL("[1]", json::array(array,1u).to_json());
    CHECK_EQUAL("[]", json::array(array,0).to_json());
}

TEST(json_array_copy_from_test)
{
    const json::array array = json::parse("[1,2,3,4,5,6,7]").upcast<json::array>();

    CHECK_EQUAL("[1,2,3,4,5,6,7]", array.to_json());
    CHECK_EQUAL("[2,3,4,5,6,7]", json::array(array,6u, 1u).to_json());
    CHECK_EQUAL("[1,2,3,4]", json::array(array,4u, 0).to_json());
    CHECK_EQUAL("[3]", json::array(array,1u, 2u).to_json());
    CHECK_EQUAL("[]", json::array(array, 0, 0).to_json());
}

/**
 * @test make sure, a copy has in independent array of elements
 */
TEST(json_array_copy_test)
{
    const json::array array = json::parse("[1,2,3,4,5,6,7]").upcast<json::array>();
    json::array copy(array.copy());

    copy.erase(0,4);
    CHECK_EQUAL("[1,2,3,4,5,6,7]", array.to_json());
    CHECK(array != copy);
}

TEST(json_array_copy_test2)
{
    const json::array array = json::parse("[1,2,3]").upcast<json::array>();
    json::array copy(array.copy());

    copy.erase(0,3);
    CHECK_EQUAL("[1,2,3]", array.to_json());
    CHECK(array != copy);
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
    CHECK(null != t);
    CHECK(null != f);
}

/*
 * parse the given string in two parts, to test that the state of the parser is correctly stored
 * between to calls to parse()
 */
static bool split_parse(const std::string& json)
{
    assert(!json.empty());
    const json::value expected = json::parse(json.begin(), json.end());

    if ( json.size() == 1 )
        return true;

    bool result = true;

    for ( std::string::size_type i = 1u; i != json.size() && result; ++i )
    {
        const std::string s1(json, 0, i);
        const std::string s2(json, i, json.size() -i);

        json::parser p;

        try {
            p.parse(s1.begin(), s1.end());
            p.parse(s2.begin(), s2.end());
            p.flush();
        }
        catch (...)
        {
            std::cerr << "failed to parse json splitted into:\n" 
                      << s1 
                      << "\nand\n"
                      << s2
                      << std::endl;

            result = false;
        }

        if ( p.result() != expected )
        {
            result = false;

            std::cerr << "expected:\n" 
                      << expected.to_json()
                      << "\nbut got\n"
                      << p.result().to_json()
                      << std::endl;
        }
    }

    return result;
}

TEST(simple_parser_test)
{
    const char test_json[] = "[[],12.1e12,21,\"Hallo\\u1234\",{\"a\":true,\"b\":false},{},null]";

    const json::value result = json::parse(tools::begin(test_json), tools::end(test_json)-1);
    CHECK_EQUAL(test_json, result.to_json());
    CHECK(split_parse(test_json));
}

TEST(valid_numbers_test)
{
    const char* valid_numbers[] = {
        "0", "-0", "12", "9989087", "-1223", "12.1", "-0.0", "-123.433", 
        "0.00e12", "-123.89e14", "-123.7e-1", "123e0", "0e0", "0e-0", "1.123e-1",
        "0.00E12", "-123.89E14", "-123.7E-1", "123E0", "0E0", "0E-0", "1.123E-1"
    };

    for ( const char* const * i = tools::begin(valid_numbers); i != tools::end(valid_numbers); ++i)
    {
        CHECK(split_parse(*i));
    }
}

TEST(invalid_numbers_test)
{
    const char* invalid_numbers[] = {
        "a", "b", "-", "-0.", ".12", "-1223.", ".1", 
        "0.00e", "-123.7e-", "0e", "0e+", "e"
    };

    for ( const char* const * i = tools::begin(invalid_numbers); i != tools::end(invalid_numbers); ++i)
    {
        const std::string s(*i);
        CHECK_THROW(json::parse(s.begin(), s.end()), json::parse_error);
    }
}

TEST(white_space_test)
{
    const json::value val = json::parse(" { \"f o\" : \"b a r\" , \"b \" : [ 1 , 2 , true , false ] } ");
    CHECK_EQUAL(json::parse("{\"f o\":\"b a r\",\"b \":[1,2,true,false]}"), val);
}

TEST(equality_test)
{
    using namespace json;
    const   number      eins(1);
    const   number      zwei(2);
    const   string      test("test");
    const   string      foo("foo");
    const   true_val    True;
    const   false_val   False;
    const   null        nix;
    const   object      empty_obj;
    const   array       empty_ar;

    object  obj;
    array   ar;

    obj.add(foo, nix);
    ar.add(null()).add(zwei);

    CHECK_EQUAL(eins, eins);
    CHECK_EQUAL(test, test);
    CHECK_EQUAL(True, True);
    CHECK_EQUAL(False, False);
    CHECK_EQUAL(nix, nix);
    CHECK_EQUAL(empty_obj, empty_obj);
    CHECK_EQUAL(empty_ar, empty_ar);
    CHECK_EQUAL(obj, obj);
    CHECK_EQUAL(ar, ar);

    CHECK_EQUAL(eins, number(1));
    CHECK_EQUAL(test, string("test"));
    CHECK_EQUAL(True, true_val());
    CHECK_EQUAL(False, false_val());
    CHECK_EQUAL(nix, null());
    CHECK_EQUAL(empty_obj, object());
    CHECK_EQUAL(empty_ar, array());

    CHECK(eins != zwei);
    CHECK(eins < zwei || zwei < eins);
    CHECK(!(eins == zwei));
    CHECK(test != foo);
    CHECK(test < foo || foo < test);

    CHECK(True != False);
    CHECK(True < False || False < True);

    CHECK(empty_obj != empty_ar);
    CHECK(empty_obj < empty_ar || empty_ar < empty_obj);
    CHECK(empty_obj != obj);
    CHECK(empty_ar != ar);

    CHECK(eins != nix);
    CHECK(zwei != foo);
    CHECK(True != obj);

    object  obj2;
    array   ar2;

    obj2.add(foo, nix);
    ar2.add(null()).add(zwei);

    CHECK_EQUAL(obj, obj2);
    CHECK_EQUAL(ar, ar2);
}

TEST(array_test)
{
    json::array a = json::parse("[1,2,3,4,5]").upcast<json::array>();

    CHECK_EQUAL(json::number(1), a.at(0));
    CHECK_EQUAL(json::number(2), a.at(1));
    CHECK_EQUAL(json::number(3), a.at(2));
    CHECK_EQUAL(json::number(4), a.at(3));
    CHECK_EQUAL(json::number(5), a.at(4));

    a.at(2) = json::array();
    CHECK_EQUAL(json::number(2), a.at(1));
    CHECK_EQUAL(json::array(), a.at(2));
    CHECK_EQUAL(json::number(4), a.at(3));

    a.insert(0, json::null());
    a.insert(6, json::object());
    CHECK_EQUAL("[null,1,2,[],4,5,{}]", a.to_json());

    a.erase(1, 2);
    CHECK_EQUAL("[null,[],4,5,{}]", a.to_json());

    a.erase(0, 1);
    a.erase(3, 1);
    CHECK_EQUAL("[[],4,5]", a.to_json());
}
