// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include <boost/test/unit_test.hpp>
#include "json/json.h"
#include "tools/iterators.h"
#include <iostream>

BOOST_AUTO_TEST_CASE( json_string_test )
{
    BOOST_CHECK_EQUAL(2u, json::string().size());
    BOOST_CHECK_EQUAL("\"\"", json::string().to_json());

    const json::string s1("Hallo"), s2("Hallo");
    BOOST_CHECK_EQUAL(s1, s2);
    BOOST_CHECK_EQUAL(s1, s1);

    const json::string s3;

    BOOST_CHECK(s1 != s3);
    BOOST_CHECK_EQUAL("\"Hallo\"", s1.to_json());
    
    const json::string s4("\"\\\r");
    BOOST_CHECK_EQUAL("\"\\\"\\\\\\r\"", s4.to_json());
}

BOOST_AUTO_TEST_CASE( json_empty_string_test )
{
	BOOST_CHECK( json::string().empty() );
	BOOST_CHECK( json::string( "" ).empty() );
	BOOST_CHECK( !json::string( "Foobar" ).empty() );
}

BOOST_AUTO_TEST_CASE( json_number_test )
{
    json::number zweiundvierzig(42);
    BOOST_CHECK_EQUAL("42", zweiundvierzig.to_json());
    BOOST_CHECK_EQUAL(2u, zweiundvierzig.size());

    json::number null(0);
    BOOST_CHECK_EQUAL("0", null.to_json());
    BOOST_CHECK_EQUAL(1u, null.size());

    json::number negativ(-12);
    BOOST_CHECK_EQUAL("-12", negativ.to_json());
    BOOST_CHECK_EQUAL(3u, negativ.size());
}

BOOST_AUTO_TEST_CASE( json_object_test )
{
    const json::object empty;
    BOOST_CHECK_EQUAL("{}", empty.to_json());
    BOOST_CHECK_EQUAL(2u, empty.size());

    json::object obj;
    obj.add(json::string("Hallo"), json::number(123));
    json::object inner;
    
    obj.add(json::string("inner"), inner);
    inner.add(json::string("foo"), json::string("bar"));

    BOOST_CHECK_EQUAL("{\"Hallo\":123,\"inner\":{\"foo\":\"bar\"}}", obj.to_json());
    BOOST_CHECK_EQUAL(obj.to_json().size(), obj.size());

    BOOST_CHECK_EQUAL("{\"foo\":\"bar\"}", inner.to_json());
    BOOST_CHECK_EQUAL(inner.to_json().size(), inner.size());
}

BOOST_AUTO_TEST_CASE( json_object_find_test )
{
	const json::object empty;
	BOOST_CHECK( empty.find( json::string( "key" ) ) == 0 );

	json::object obj;
    obj.add( json::string( "Hallo" ), json::number( 123 ) );

    const json::value* num = obj.find( json::string( "Hallo" ) );
    BOOST_REQUIRE( num != 0 );
    BOOST_CHECK_EQUAL( *num, json::number( 123 ) );

    json::value* not_found = obj.find( json::string( "Halloe" ) );
    BOOST_CHECK( not_found == 0 );
}

BOOST_AUTO_TEST_CASE( copy_object_test )
{
	const json::object obj = json::parse_single_quoted(
		"{"
		"	'Annette' : 1, "
		"	'Todi' : 2 "
		"}" ).upcast< json::object >();

	json::object copy = obj.copy();
	json::object same = obj;

	BOOST_REQUIRE_EQUAL( copy, obj );
	BOOST_REQUIRE_EQUAL( same, obj );

	same.add( json::string( "foo" ), json::null() );

	BOOST_CHECK_EQUAL( same, obj );
	BOOST_CHECK_NE( copy, obj );
	BOOST_CHECK_EQUAL( copy, json::parse_single_quoted(
		"{"
		"	'Annette' : 1, "
		"	'Todi' : 2 "
		"}" ).upcast< json::object >() );
}

BOOST_AUTO_TEST_CASE( json_array_test )
{
    json::array array;
    BOOST_CHECK_EQUAL("[]", array.to_json());
    BOOST_CHECK_EQUAL(2u, array.size());

    array.add(json::string("Hallo"));

    BOOST_CHECK_EQUAL("[\"Hallo\"]", array.to_json());
    BOOST_CHECK_EQUAL(array.to_json().size(), array.size());

    json::array inner;
    array.add(inner);

    inner.add(json::number(0));

    BOOST_CHECK_EQUAL("[\"Hallo\",[0]]", array.to_json());
    BOOST_CHECK_EQUAL(array.to_json().size(), array.size());
}

BOOST_AUTO_TEST_CASE( json_array_copy_at_begin_test )
{
    const json::array array = json::parse("[1,2,3,4,5,6,7]").upcast<json::array>();

    BOOST_CHECK_EQUAL("[1,2,3,4,5,6,7]", array.to_json());
    BOOST_CHECK_EQUAL("[1,2,3,4,5,6,7]", json::array(array,7u).to_json());
    BOOST_CHECK_EQUAL("[1,2,3,4]", json::array(array,4u).to_json());
    BOOST_CHECK_EQUAL("[1]", json::array(array,1u).to_json());
    BOOST_CHECK_EQUAL("[]", json::array(array,0).to_json());
}

BOOST_AUTO_TEST_CASE( json_array_copy_from_test )
{
    const json::array array = json::parse("[1,2,3,4,5,6,7]").upcast<json::array>();

    BOOST_CHECK_EQUAL("[1,2,3,4,5,6,7]", array.to_json());
    BOOST_CHECK_EQUAL("[2,3,4,5,6,7]", json::array(array,6u, 1u).to_json());
    BOOST_CHECK_EQUAL("[1,2,3,4]", json::array(array,4u, 0).to_json());
    BOOST_CHECK_EQUAL("[3]", json::array(array,1u, 2u).to_json());
    BOOST_CHECK_EQUAL("[]", json::array(array, 0, 0).to_json());
}

/**
 * @test make sure, a copy has in independent array of elements
 */
BOOST_AUTO_TEST_CASE( json_array_copy_test )
{
    const json::array array = json::parse("[1,2,3,4,5,6,7]").upcast<json::array>();
    json::array copy(array.copy());

    copy.erase(0,4);
    BOOST_CHECK_EQUAL("[1,2,3,4,5,6,7]", array.to_json());
    BOOST_CHECK(array != copy);
}

BOOST_AUTO_TEST_CASE( json_array_copy_test2 )
{
    const json::array array = json::parse("[1,2,3]").upcast<json::array>();
    json::array copy(array.copy());

    copy.erase(0,3);
    BOOST_CHECK_EQUAL("[1,2,3]", array.to_json());
    BOOST_CHECK(array != copy);
}

BOOST_AUTO_TEST_CASE( json_special_test )
{
    json::value null = json::null();
    BOOST_CHECK_EQUAL(json::null(), null);
    BOOST_CHECK_EQUAL("null", null.to_json());
    BOOST_CHECK_EQUAL(null.to_json().size(), null.size());

    json::false_val f;
    BOOST_CHECK_EQUAL("false", f.to_json());
    BOOST_CHECK_EQUAL(f.to_json().size(), f.size());

    json::true_val t;
    BOOST_CHECK_EQUAL("true", t.to_json());
    BOOST_CHECK_EQUAL(t.to_json().size(), t.size());

    BOOST_CHECK(t != f);
    BOOST_CHECK(null != t);
    BOOST_CHECK(null != f);
}

/*
 * parse the given string in two parts, to test that the state of the parser is correctly stored
 * between two calls to parse()
 */
static bool split_parse( const std::string& json )
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

BOOST_AUTO_TEST_CASE( simple_parser_test )
{
    const char test_json[] = "[[],12.1e12,21,\"Hallo\\u1234\",{\"a\":true,\"b\":false},{},null]";

    const json::value result = json::parse(tools::begin(test_json), tools::end(test_json)-1);
    BOOST_CHECK_EQUAL(test_json, result.to_json());
    BOOST_CHECK(split_parse(test_json));
}

BOOST_AUTO_TEST_CASE( valid_numbers_test )
{
    const char* valid_numbers[] = {
        "0", "-0", "12", "9989087", "-1223", "12.1", "-0.0", "-123.433", 
        "0.00e12", "-123.89e14", "-123.7e-1", "123e0", "0e0", "0e-0", "1.123e-1",
        "0.00E12", "-123.89E14", "-123.7E-1", "123E0", "0E0", "0E-0", "1.123E-1"
    };

    for ( const char* const * i = tools::begin(valid_numbers); i != tools::end(valid_numbers); ++i)
    {
        BOOST_CHECK(split_parse(*i));
    }
}

BOOST_AUTO_TEST_CASE( invalid_numbers_test )
{
    const char* invalid_numbers[] = {
        "a", "b", "-", "-0.", ".12", "-1223.", ".1", 
        "0.00e", "-123.7e-", "0e", "0e+", "e"
    };

    for ( const char* const * i = tools::begin(invalid_numbers); i != tools::end(invalid_numbers); ++i)
    {
        const std::string s(*i);
        BOOST_CHECK_THROW(json::parse(s.begin(), s.end()), json::parse_error);
    }
}

BOOST_AUTO_TEST_CASE( white_space_test )
{
    const json::value val = json::parse(" { \"f o\" : \"b a r\" , \"b \" : [ 1 , 2 , true , false ] } ");
    BOOST_CHECK_EQUAL(json::parse("{\"f o\":\"b a r\",\"b \":[1,2,true,false]}"), val);
}

BOOST_AUTO_TEST_CASE( equality_test )
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

    BOOST_CHECK_EQUAL(eins, eins);
    BOOST_CHECK_EQUAL(test, test);
    BOOST_CHECK_EQUAL(True, True);
    BOOST_CHECK_EQUAL(False, False);
    BOOST_CHECK_EQUAL(nix, nix);
    BOOST_CHECK_EQUAL(empty_obj, empty_obj);
    BOOST_CHECK_EQUAL(empty_ar, empty_ar);
    BOOST_CHECK_EQUAL(obj, obj);
    BOOST_CHECK_EQUAL(ar, ar);

    BOOST_CHECK_EQUAL(eins, number(1));
    BOOST_CHECK_EQUAL(test, string("test"));
    BOOST_CHECK_EQUAL(True, true_val());
    BOOST_CHECK_EQUAL(False, false_val());
    BOOST_CHECK_EQUAL(nix, null());
    BOOST_CHECK_EQUAL(empty_obj, object());
    BOOST_CHECK_EQUAL(empty_ar, array());

    BOOST_CHECK(eins != zwei);
    BOOST_CHECK(eins < zwei || zwei < eins);
    BOOST_CHECK(!(eins == zwei));
    BOOST_CHECK(test != foo);
    BOOST_CHECK(test < foo || foo < test);

    BOOST_CHECK(True != False);
    BOOST_CHECK(True < False || False < True);

    BOOST_CHECK(empty_obj != empty_ar);
    BOOST_CHECK(empty_obj < empty_ar || empty_ar < empty_obj);
    BOOST_CHECK(empty_obj != obj);
    BOOST_CHECK(empty_ar != ar);

    BOOST_CHECK(eins != nix);
    BOOST_CHECK(zwei != foo);
    BOOST_CHECK(True != obj);

    object  obj2;
    array   ar2;

    obj2.add(foo, nix);
    ar2.add(null()).add(zwei);

    BOOST_CHECK_EQUAL(obj, obj2);
    BOOST_CHECK_EQUAL(ar, ar2);
}

BOOST_AUTO_TEST_CASE( array_test )
{
    json::array a = json::parse("[1,2,3,4,5]").upcast<json::array>();

    BOOST_CHECK_EQUAL(json::number(1), a.at(0));
    BOOST_CHECK_EQUAL(json::number(2), a.at(1));
    BOOST_CHECK_EQUAL(json::number(3), a.at(2));
    BOOST_CHECK_EQUAL(json::number(4), a.at(3));
    BOOST_CHECK_EQUAL(json::number(5), a.at(4));

    a.at(2) = json::array();
    BOOST_CHECK_EQUAL(json::number(2), a.at(1));
    BOOST_CHECK_EQUAL(json::array(), a.at(2));
    BOOST_CHECK_EQUAL(json::number(4), a.at(3));

    a.insert(0, json::null());
    a.insert(6, json::object());
    BOOST_CHECK_EQUAL("[null,1,2,[],4,5,{}]", a.to_json());

    a.erase(1, 2);
    BOOST_CHECK_EQUAL("[null,[],4,5,{}]", a.to_json());

    a.erase(0, 1);
    a.erase(3, 1);
    BOOST_CHECK_EQUAL("[[],4,5]", a.to_json());
}

/**
 * @test check, that array::for_each works a expected
 */
BOOST_AUTO_TEST_CASE( array_for_each )
{
    json::array a = json::parse("[{\"a\":2},[1,2,3],\"2\",3,4,5,false,true,null]").upcast<json::array>();

    struct : json::visitor
    {
        void visit( const json::string& o )
        {
        	result.add( o );
        }

        void visit( const json::number& o )
        {
        	result.add( o );
        }

        void visit( const json::object& o )
        {
        	result.add( o );
        }

        void visit( const json::array& o )
        {
        	result.add( o );
        }

        void visit( const json::true_val& o )
        {
        	result.add( o );
        }

        void visit( const json::false_val& o )
        {
        	result.add( o );
        }

        void visit( const json::null& o )
        {
        	result.add( o );
        }

        json::array result;
    } v;

    a.for_each( v );
    BOOST_CHECK_EQUAL( a, v.result );
}

/**
 * @test conversion from std::string to json::string and back must be transparent and result in the very same text
 */
BOOST_AUTO_TEST_CASE( convert_json_string_to_std_string )
{

	const char * const test_texts[] = {
		"Hallo",
		"\\",
		"\"\\/\b\f\n\r\t",
		"1.2.3, adasd |{}[\\\\"
	};

	for ( char const * const * test = tools::begin( test_texts ); test != tools::end( test_texts ); ++test )
		BOOST_CHECK_EQUAL( std::string( *test ), json::string( *test ).to_std_string() );
}

bool not_equal( const json::value& lhs, const json::value& rhs )
{
	return !( lhs < rhs ) && !( rhs < lhs );
}

/**
 * @test strings that are escaped have to compare equal, if there content is equal, independent from different
 *       escapings.
 */
BOOST_AUTO_TEST_CASE( json_string_compare_test )
{
	// without C escaping: "/" == "\/"
	BOOST_CHECK_EQUAL( json::parse( "\"/\"" ), json::parse( "\"\\/\"" ) );
	BOOST_CHECK( not_equal( json::parse( "\"/\"" ), json::parse( "\"\\/\"" ) ) );
}

/**
 * @test tests the object::empty() function
 */
BOOST_AUTO_TEST_CASE( object_empty_test )
{
    json::object foo;
    BOOST_CHECK( foo.empty() );

    foo.add( json::string("a"), json::null() );
    BOOST_CHECK( !foo.empty() );

    foo.add( json::string("b"), json::number( 12 ) );
    BOOST_CHECK( !foo.empty() );

    foo.erase( json::string("a") );
    BOOST_CHECK( !foo.empty() );

    foo.erase( json::string("b") );
    BOOST_CHECK( foo.empty() );
}

/**
 * @test test the value::try_cast() function
 */
BOOST_AUTO_TEST_CASE( value_try_cast )
{
    const json::string str( "hallo" );
    const json::number num( 42 );
    const json::object obj;
    const json::array  arr( json::string( "foo" ) );
    const json::true_val true_;
    const json::false_val false_;
    const json::null   null;

    const std::pair< bool, json::string > string_as_string = json::value( str ).try_cast< json::string >();
    const std::pair< bool, json::number > string_as_number = json::value( str ).try_cast< json::number >();
    const std::pair< bool, json::null >   string_as_null   = json::value( str ).try_cast< json::null >();

    BOOST_CHECK_EQUAL( str, string_as_string.second );
    BOOST_CHECK( string_as_string.first );
    BOOST_CHECK( !string_as_number.first );
    BOOST_CHECK( !string_as_null.first );

    const std::pair< bool, json::number > number_as_number = json::value( num ).try_cast< json::number >();
    const std::pair< bool, json::string > number_as_string = json::value( num ).try_cast< json::string >();
    const std::pair< bool, json::array >  number_as_array  = json::value( num ).try_cast< json::array >();

    BOOST_CHECK_EQUAL( num, number_as_number.second );
    BOOST_CHECK( number_as_number.first );
    BOOST_CHECK( !number_as_string.first );
    BOOST_CHECK( !number_as_array.first );

    const std::pair< bool, json::object >    object_as_object = json::value( obj ).try_cast< json::object >();
    const std::pair< bool, json::true_val >  object_as_true   = json::value( obj ).try_cast< json::true_val >();
    const std::pair< bool, json::false_val > object_as_false  = json::value( obj ).try_cast< json::false_val >();

    BOOST_CHECK_EQUAL( obj, object_as_object.second );
    BOOST_CHECK( object_as_object.first );
    BOOST_CHECK( !object_as_true.first );
    BOOST_CHECK( !object_as_false.first );

    const std::pair< bool, json::array >  array_as_array  = json::value( arr ).try_cast< json::array >();
    const std::pair< bool, json::object > array_as_object = json::value( arr ).try_cast< json::object >();
    const std::pair< bool, json::null >   array_as_null   = json::value( arr ).try_cast< json::null >();

    BOOST_CHECK_EQUAL( arr, array_as_array.second );
    BOOST_CHECK( array_as_array.first );
    BOOST_CHECK( !array_as_object.first );
    BOOST_CHECK( !array_as_null.first );

    const std::pair< bool, json::true_val >  true_as_true   = json::value( true_ ).try_cast< json::true_val >();
    const std::pair< bool, json::false_val > true_as_false  = json::value( true_ ).try_cast< json::false_val >();
    const std::pair< bool, json::string >    true_as_string = json::value( true_ ).try_cast< json::string >();

    BOOST_CHECK_EQUAL( true_, true_as_true.second );
    BOOST_CHECK( true_as_true.first );
    BOOST_CHECK( !true_as_false.first );
    BOOST_CHECK( !true_as_string.first );

    const std::pair< bool, json::false_val > false_as_false  = json::value( false_ ).try_cast< json::false_val >();
    const std::pair< bool, json::number >    false_as_number = json::value( false_ ).try_cast< json::number >();
    const std::pair< bool, json::null >      false_as_null   = json::value( false_ ).try_cast< json::null >();

    BOOST_CHECK_EQUAL( false_, false_as_false.second );
    BOOST_CHECK( false_as_false.first );
    BOOST_CHECK( !false_as_number.first );
    BOOST_CHECK( !false_as_null.first );

    const std::pair< bool, json::null >   null_as_null   = json::value( null ).try_cast< json::null >();
    const std::pair< bool, json::number > null_as_number = json::value( null ).try_cast< json::number >();
    const std::pair< bool, json::string > null_as_string = json::value( null ).try_cast< json::string >();

    BOOST_CHECK_EQUAL( null, null_as_null.second );
    BOOST_CHECK( null_as_null.first );
    BOOST_CHECK( !null_as_number.first );
    BOOST_CHECK( !null_as_string.first );
}

