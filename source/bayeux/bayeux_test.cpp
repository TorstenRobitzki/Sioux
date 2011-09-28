// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#define BOOST_TEST_MAIN

#include <algorithm>
#include <boost/test/unit_test.hpp>

#include "bayeux/bayeux.h"
#include "server/test_socket.h"
#include "server/test_io_plan.h"
#include "tools/asstring.h"

namespace
{
	typedef server::test::socket< const char* > socket_t;

	template < std::size_t S >
	std::string bayeux_msg( const char(&txt)[S])
	{
		std::string body( txt );
		std::replace( body.begin(), body.end(), '\'', '\"' );

		const std::string result =
			"POST / HTTP/1.1\r\n"
			"Host: bayeux-server.de\r\n"
			"Content-Type: application/json\r\n"
			"Content-Length: "
		 + tools::as_string( body.size() )
		 + "\r\n\r\n"
		 +  body;

		return result;
	}
}

/**
 * @test simulate a connect to the server, a subscription to a node with value and a disconnect
 */
BOOST_AUTO_TEST_CASE( handshake_connect_subscribe_disconnect )
{
	server::test::read_plan input;
	input << server::test::read( bayeux_msg(
			"{ 'channel' : '/meta/handshake',"
			"  'version' : '1.0.0',"
			"  'supportedConnectionTypes' : ['long-polling', 'callback-polling', 'iframe'] }" ) );

	boost::asio::io_service	queue;
	socket_t				socket( queue, input );
}

/**
 * @test between a handshake and a connect, the
 */
BOOST_AUTO_TEST_CASE( handshake_break_connect_subscribe_disconnect )
{
}

/**
 * @test what should happen, if none of the enumerated connection type is supported by the server
 */
BOOST_AUTO_TEST_CASE( no_supported_connection_type )
{
}

/**
 * @brief hurry a listening connection, if a normal http request is pipelined
 */
BOOST_AUTO_TEST_CASE( hurry_bayeux_connection_if_request_is_pipelined )
{
}

BOOST_AUTO_TEST_CASE( single_valued_containing_a_single_bayeux_message )
{
}

BOOST_AUTO_TEST_CASE( single_valued_containing_an_array_of_bayeux_messages )
{
}

BOOST_AUTO_TEST_CASE( multi_valued_containing_a_several_invidiual_bayeux_message )
{
}

BOOST_AUTO_TEST_CASE( multi_valued_containing_a_several_arrays_of_bayeux_messages )
{
}

BOOST_AUTO_TEST_CASE( multi_valued_containing_a_mix_of_invidiual_bayeux_messages_and_array )
{
}
