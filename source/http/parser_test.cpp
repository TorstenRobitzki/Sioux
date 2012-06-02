// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "http/parser.h"

#include "tools/iterators.h"

#include <boost/test/unit_test.hpp>

// hack to get the 0xff into the string literal
static const unsigned char expected_raw_text[] = { 0x20, 'a', 'b', 'c', 0xff, 0, 0x7f };
static const char*         as_char_array = static_cast< const char* >( static_cast< const void* >( &expected_raw_text[ 0 ] ) );
static const std::string   expected_text( as_char_array, as_char_array + sizeof expected_raw_text );

BOOST_AUTO_TEST_CASE( url_decode_test )
{

    BOOST_CHECK_EQUAL( http::url_decode( "%20abc%ff%00%7f" ), expected_text );
}

BOOST_AUTO_TEST_CASE( url_decode_error )
{
    BOOST_CHECK_THROW( http::url_decode( "abc%" ), http::bad_url );
    BOOST_CHECK_THROW( http::url_decode( "abc%a" ), http::bad_url );
    BOOST_CHECK_THROW( http::url_decode( "%1wab" ), http::bad_url );
    BOOST_CHECK_THROW( http::url_decode( "a%l1as" ), http::bad_url );
}

BOOST_AUTO_TEST_CASE( url_decode_substring )
{
    std::string       memory( "%20abc%ff%00%7f" );
    tools::substring  text( memory.c_str(), memory.c_str() + memory.size() );

    BOOST_CHECK_EQUAL( http::url_decode( text ), expected_text );
}

BOOST_AUTO_TEST_CASE( url_decode_mixed_case_test )
{
    BOOST_CHECK_EQUAL( http::url_decode( "%4A%4b%4C%4d%4E" ), "JKLMN" );
}


BOOST_AUTO_TEST_CASE( url_encode_test )
{
    BOOST_CHECK_EQUAL( http::url_encode( expected_text ), "%20abc%FF%00%7F" );
}

// all but "-_.~" are special characters
BOOST_AUTO_TEST_CASE( url_encode_all_specials_test )
{
    BOOST_CHECK_EQUAL( http::url_encode( "!\"@$%&/\\" ), "%21%22%40%24%25%26%2F%5C" );
}

BOOST_AUTO_TEST_CASE( url_encode_all_non_specials_test )
{
    static const std::string text =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        "-_.~";
    BOOST_CHECK_EQUAL( http::url_encode( text ), text );
}

static const std::string test_url( "http://joe:passwd@www.example.net:8080/index.html?action=something&session=A54C6FE2#info" );

BOOST_AUTO_TEST_CASE( split_url_test_std_string )
{
    std::string scheme, authority, path, query, fragment;
    http::split_url( test_url, scheme, authority, path, query, fragment );

    BOOST_CHECK_EQUAL( scheme, "http" );
    BOOST_CHECK_EQUAL( authority, "joe:passwd@www.example.net:8080" );
    BOOST_CHECK_EQUAL( path, "/index.html" );
    BOOST_CHECK_EQUAL( query, "action=something&session=A54C6FE2" );
    BOOST_CHECK_EQUAL( fragment, "info" );
}

BOOST_AUTO_TEST_CASE( split_url_test_tool_substring )
{
    tools::substring scheme, authority, path, query, fragment;
    const tools::substring url( test_url.c_str(), test_url.c_str() + test_url.size() );

    http::split_url( url, scheme, authority, path, query, fragment );

    BOOST_CHECK_EQUAL( scheme, "http" );
    BOOST_CHECK_EQUAL( authority, "joe:passwd@www.example.net:8080" );
    BOOST_CHECK_EQUAL( path, "/index.html" );
    BOOST_CHECK_EQUAL( query, "action=something&session=A54C6FE2" );
    BOOST_CHECK_EQUAL( fragment, "info" );
}

namespace {
    template < unsigned Size >
    tools::substring substr( const char ( &text )[ Size ] )
    {
        return tools::substring( &text[ 0 ], &text[ Size - 1 ] );
    }
}

BOOST_AUTO_TEST_CASE( split_query )
{
    const char query[] = "action=%20foo%20&message=a+bs&key=&name=foobar";

    std::vector< std::pair< tools::substring, tools::substring > > result =
        http::split_query( substr( query ) );

    BOOST_CHECK_EQUAL( result.size(), 4u );
    BOOST_CHECK_EQUAL( result[ 0 ].first, "action" );
    BOOST_CHECK_EQUAL( result[ 0 ].second, "%20foo%20" );
    BOOST_CHECK_EQUAL( result[ 1 ].first, "message" );
    BOOST_CHECK_EQUAL( result[ 1 ].second, "a+bs" );
    BOOST_CHECK_EQUAL( result[ 2 ].first, "key" );
    BOOST_CHECK_EQUAL( result[ 2 ].second, "" );
    BOOST_CHECK_EQUAL( result[ 3 ].first, "name" );
    BOOST_CHECK_EQUAL( result[ 3 ].second, "foobar" );
}

BOOST_AUTO_TEST_CASE( split_query_with_mulitple_names )
{
    const char query[] = "p3=f&p1=a&p1=b&p2=c&p3=f";

    std::vector< std::pair< tools::substring, tools::substring > > result =
        http::split_query( substr( query ) );

    BOOST_CHECK_EQUAL( result.size(), 5u );
    BOOST_CHECK_EQUAL( result[ 0 ].first, "p3" );
    BOOST_CHECK_EQUAL( result[ 0 ].second, "f" );
    BOOST_CHECK_EQUAL( result[ 1 ].first, "p1" );
    BOOST_CHECK_EQUAL( result[ 1 ].second, "a" );
    BOOST_CHECK_EQUAL( result[ 2 ].first, "p1" );
    BOOST_CHECK_EQUAL( result[ 2 ].second, "b" );
    BOOST_CHECK_EQUAL( result[ 3 ].first, "p2" );
    BOOST_CHECK_EQUAL( result[ 3 ].second, "c" );
    BOOST_CHECK_EQUAL( result[ 4 ].first, "p3" );
    BOOST_CHECK_EQUAL( result[ 4 ].second, "f" );
}

BOOST_AUTO_TEST_CASE( split_empty_query )
{
    BOOST_CHECK_EQUAL( http::split_query( tools::substring() ).size(), 0 );
}

BOOST_AUTO_TEST_CASE( split_buggy_query )
{
    const char query1[] = "action=%20foo%20&messageabs&key=";
    const char query2[] = "&";

    BOOST_CHECK_THROW( http::split_query( substr( query1 ) ), http::bad_query );
    BOOST_CHECK_THROW( http::split_query( substr( query2 ) ), http::bad_query );
}

BOOST_AUTO_TEST_CASE( form_decode_test )
{
    const char query[] = "+%20%4A%4b%2B";

    const std::string result = http::form_decode( substr( query ) );

    BOOST_CHECK_EQUAL( result, "  JK+");
}
