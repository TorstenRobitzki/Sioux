// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include <boost/test/unit_test.hpp>
#include <boost/asio/buffers_iterator.hpp>

#include "http/decode_stream.h"
#include "http/request.h"
#include "http/response.h"
#include "http/test_request_texts.h"
#include "tools/iterators.h"

BOOST_AUTO_TEST_CASE( empty_stream_empty_result )
{
	BOOST_CHECK( http::decode_stream< http::request_header >( std::vector< char >() ).empty() );
	BOOST_CHECK( http::decode_stream< http::response_header >( std::vector< char >() ).empty() );
}

BOOST_AUTO_TEST_CASE( split_a_simple_request_into_body_and_header )
{
	const std::vector< char > stream(
		tools::begin( http::test::simple_post ), tools::end( http::test::simple_post ) -1 );

	const http::decoded_request_stream_t bodies_and_headers = http::decode_stream< http::request_header >( stream );

	BOOST_REQUIRE_EQUAL( 1u, bodies_and_headers.size() );
	const std::vector< char > body( bodies_and_headers.front().second );

	BOOST_CHECK_EQUAL( "url=http%3A%2F%2Fasdasdasd&submit=Submit&http=1.1&gzip=yes&type=GET&uak=0",
		std::string( body.begin(), body.end() ) );
}

BOOST_AUTO_TEST_CASE( multiple_request_with_body_and_header )
{
	const unsigned repititions = 100;

	std::vector< char > stream;
	for ( unsigned i = 0; i != repititions; ++i )
		stream.insert(
			stream.end(), tools::begin( http::test::simple_post ), tools::end( http::test::simple_post ) -1 );

	const http::decoded_request_stream_t bodies_and_headers = http::decode_stream< http::request_header >( stream );

	BOOST_REQUIRE_EQUAL( repititions, bodies_and_headers.size() );

	for ( http::decoded_request_stream_t::const_iterator pair = bodies_and_headers.begin();
			pair != bodies_and_headers.end(); ++pair )
	{
		const std::vector< char > body( pair->second );

		BOOST_CHECK_EQUAL( "url=http%3A%2F%2Fasdasdasd&submit=Submit&http=1.1&gzip=yes&type=GET&uak=0",
			std::string( body.begin(), body.end() ) );
	}
}
