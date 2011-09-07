// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include <boost/test/unit_test.hpp>
#include <sstream>
#include "pubsub/connector.h"
#include "server/test_socket.h"
#include "http/request.h"

typedef server::test::socket< const char* > socket_t;

namespace
{
	boost::shared_ptr< server::async_response > create_response( const std::string& message_text )
	{
		const char* const header =
			"POST / HTTP/1.1\r\n"
			"Host: pubsub.de\r\n"
			"User-Agent: Web-sniffer/1.0.31 (+http://web-sniffer.net/)\r\n"
			"Accept-Encoding: gzip\r\n"
			"Accept-Charset: ISO-8859-1,UTF-8;q=0.7,*;q=0.7\r\n"
			"Cache-Control: no\r\n"
			"Accept-Language: de,en;q=0.7,en-us;q=0.3\r\n"
			"Referer: http://web-sniffer.net/\r\n"
			"Content-Length: ";

		std::ostringstream out_buffer;
		out_buffer << header << message_text.size() << "\r\n\r\n" << message_text;

		return boost::shared_ptr< server::async_response >();
	}

	std::string create_response_text( const char* message_text )
	{
		boost::shared_ptr< server::async_response > response = create_response( message_text );

	}
}

/**
 * @test a new user subscribes to a node, without session id and without version.
 *
 * The client must get a session id and the current version of the data.
 */
BOOST_AUTO_TEST_CASE( first_subscription_over_http )
{
	const boost::shared_ptr< server::async_response > response = create_response(
		"{ \"subscribe\" : { \"a\" : 1, \"b\" :  2 } }" );

}

/**
 * @test after a client got it's first, initial data, it must get a newer versions of the data, if the data changes
 *
 */
BOOST_AUTO_TEST_CASE( update_over_http )
{
}

/**
 * @test check, that the server will response with an error, when the maximum number of
 *        subscriptions is reached.
 */
BOOST_AUTO_TEST_CASE( maximum_number_of_subscriptions_reached )
{
}
