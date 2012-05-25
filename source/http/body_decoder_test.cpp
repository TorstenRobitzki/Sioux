// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include <boost/test/unit_test.hpp>
#include "http/body_decoder.h"
#include "http/request.h"
#include "http/test_request_texts.h"

static const http::request_header header_with_body(
		"POST / HTTP/1.1\r\n"
		"Host: google.de\r\n"
		"User-Agent: Web-sniffer/1.0.31 (+http://web-sniffer.net/)\r\n"
		"Accept-Encoding: gzip\r\n"
		"Accept-Charset: ISO-8859-1,UTF-8;q=0.7,*;q=0.7\r\n"
		"Cache-Control: no\r\n"
		"Accept-Language: de,en;q=0.7,en-us;q=0.3\r\n"
		"Referer: http://web-sniffer.net/\r\n"
		"Content-Length: 5\r\n"
		"\r\n"
		"12345"
		"POST / HTTP/1.1\r\n" // next request
		"Host: google.de\r\n"
	);

BOOST_AUTO_TEST_CASE( test_header_with_body )
{
	BOOST_REQUIRE( header_with_body.unparsed_buffer().first != 0 );
	BOOST_CHECK_EQUAL( '1', *header_with_body.unparsed_buffer().first );
	BOOST_CHECK( header_with_body.unparsed_buffer().second > 5u );
}

BOOST_AUTO_TEST_CASE( decode_length_encoded )
{
	http::body_decoder decoder;
	BOOST_CHECK_EQUAL( http::http_ok, decoder.start( header_with_body ) );

	// when feeding to the decoder, only the 5 Bytes body must be consumed
	BOOST_CHECK_EQUAL( 5u, decoder.feed_buffer( header_with_body.unparsed_buffer().first, header_with_body.unparsed_buffer().second ) );

	// first call to decode() must return the feed buffer
	const std::pair< std::size_t, const char* > decoded = decoder.decode();

	BOOST_CHECK_EQUAL( 5u, decoded.first );
	BOOST_REQUIRE( decoded.second != 0 );
	BOOST_CHECK_EQUAL( "12345", tools::substring( decoded.second, decoded.second + 5u ) );

	BOOST_CHECK_EQUAL( 0, decoder.decode().first );
	BOOST_CHECK_EQUAL( 0, decoder.feed_buffer( header_with_body.unparsed_buffer().first, header_with_body.unparsed_buffer().second ) );
}

BOOST_AUTO_TEST_CASE( decode_length_encoded_step_by_step )
{
	http::body_decoder decoder;
	BOOST_CHECK_EQUAL( http::http_ok, decoder.start( header_with_body ) );

	std::pair<const char*, std::size_t> buffer = header_with_body.unparsed_buffer();

	// feeding only the first 3 bytes
	BOOST_CHECK_EQUAL( 3u, decoder.feed_buffer( buffer.first, 3u ) );

	// first call to decode() must return the feed buffer
	std::pair< std::size_t, const char* > decoded = decoder.decode();

	BOOST_CHECK_EQUAL( 3u, decoded.first );
	BOOST_REQUIRE( decoded.second != 0 );
	BOOST_CHECK_EQUAL( "123", tools::substring( decoded.second, decoded.second + 3u ) );

	BOOST_CHECK_EQUAL( 0, decoder.decode().first );

	// feeding the second 3 bytes
	BOOST_CHECK_EQUAL( 2u, decoder.feed_buffer( buffer.first + decoded.first, 3u ) );

	decoded = decoder.decode();

	BOOST_CHECK_EQUAL( 2u, decoded.first );
	BOOST_REQUIRE( decoded.second != 0 );
	BOOST_CHECK_EQUAL( "45", tools::substring( decoded.second, decoded.second + 2u ) );

	BOOST_CHECK_EQUAL( 0, decoder.decode().first );
	BOOST_CHECK_EQUAL( 0, decoder.feed_buffer( buffer.first + 5u, 3u ) );
}

BOOST_AUTO_TEST_CASE( body_decoder_with_illegal_size )
{
	const http::request_header header(
			"POST / HTTP/1.1\r\n"
			"Host: google.de\r\n"
			"Content-Length: kaput\r\n"
			"\r\n"
			"12345");

	http::body_decoder decoder;
	BOOST_CHECK_EQUAL( http::http_bad_request, decoder.start( header ) );
}

BOOST_AUTO_TEST_CASE( body_decoder_without_length_header )
{
	http::body_decoder decoder;
	BOOST_CHECK_EQUAL( http::http_length_required, decoder.start( http::request_header( http::test::simple_get_11 ) ) );
}

BOOST_AUTO_TEST_CASE( test_header_with_empty_body )
{
    const http::request_header header(
            "POST / HTTP/1.1\r\n"
            "Host: google.de\r\n"
            "Content-Length: 0\r\n"
            "\r\n\r\n" );

    http::body_decoder decoder;
    BOOST_CHECK_EQUAL( http::http_ok, decoder.start( header ) );

    BOOST_CHECK_EQUAL( 0, decoder.feed_buffer( header_with_body.unparsed_buffer().first, header_with_body.unparsed_buffer().second ) );

    // first call to decode() must return the feed buffer
    const std::pair< std::size_t, const char* > decoded = decoder.decode();

    BOOST_CHECK_EQUAL( 0u, decoded.first );
    BOOST_REQUIRE( decoded.second != 0 );
}

