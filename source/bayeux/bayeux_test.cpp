// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#define BOOST_TEST_MAIN

#include <algorithm>
#include <boost/test/unit_test.hpp>

#include "bayeux/bayeux.h"
#include "bayeux/configuration.h"
#include "http/request.h"
#include "http/response.h"
#include "http/decode_stream.h"
#include "pubsub/configuration.h"
#include "pubsub/root.h"
#include "pubsub/test_helper.h"
#include "server/connection.h"
#include "server/error.h"
#include "server/log.h"
#include "server/test_io_plan.h"
#include "server/test_session_generator.h"
#include "server/test_socket.h"
#include "server/traits.h"
#include "tools/asstring.h"
#include "tools/io_service.h"

namespace
{
	typedef server::test::socket< const char* > socket_t;

	template < std::size_t S >
	server::test::read bayeux_msg( const char(&txt)[S] )
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

		return server::test::read( result.begin(), result.end() );
	}

	struct response_factory
	{
		template < class Trait >
	    explicit response_factory( Trait& trait )
	      : connector_( trait.data(), session_generator_, trait.config() )
		{
		}

	    template < class Connection >
	    boost::shared_ptr< server::async_response > create_response(
	        const boost::shared_ptr< Connection >&                    connection,
	        const boost::shared_ptr< const http::request_header >&    header )
	    {
	    	if ( header->state() == http::message::ok )
	    	{
	    		return connector_.create_response( connection, header );
	    	}

	    	return boost::shared_ptr< server::async_response >(
	    			new server::error_response< Connection >( connection, http::http_bad_request ) );
	    }

	    template < class Connection >
        boost::shared_ptr< server::async_response > error_response(
        	const boost::shared_ptr< Connection >& 	con,
        	http::http_error_code					ec )
        {
            return boost::shared_ptr< server::async_response >( new ::server::error_response< Connection >( con, ec ) );
        }

	    bayeux::connector				connector_;
	    server::test::session_generator	session_generator_;
	};

	class trait_data
	{
	public:
		trait_data( pubsub::root& data, const bayeux::configuration& config )
			: data_( data )
			, config_( config )
		{
		}

		std::ostream& logstream() const
		{
			return std::cerr;
		}

		pubsub::root& data() const
		{
			return data_;
		}

		const bayeux::configuration& config() const
		{
			return config_;
		}
	private:
		pubsub::root& 			data_;
		bayeux::configuration 	config_;
	};

	typedef server::test::socket<const char*>                       socket_t;
	typedef server::null_event_logger								event_logger_t;
//	typedef server::stream_event_log								event_logger_t;
	typedef server::stream_error_log								error_logger_t;

	class trait_t :
		public trait_data,
		public server::connection_traits< socket_t, response_factory, event_logger_t, error_logger_t >
	{
	public:
		typedef server::connection_traits< socket_t, response_factory, event_logger_t, error_logger_t > base_t;

		explicit trait_t( pubsub::root& data, const bayeux::configuration& config = bayeux::configuration() )
			: trait_data( data, config )
			, base_t( *this )
		{
		}
	};

	struct test_context
	{
		boost::asio::io_service	queue;
		pubsub::test::adapter	adapter;
		pubsub::root			data;
		trait_t					trait;
		socket_t				socket;

		test_context()
			: queue()
			, adapter()
			, data( queue, adapter, pubsub::configuration() )
			, trait( data )
		{
		}
	};

	typedef server::connection< trait_t >                           connection_t;
	typedef std::pair< boost::shared_ptr< http::response_header >, json::array >	response_t;

	std::vector< response_t > extract_responses( const std::vector< char >& response_text )
	{
		const http::decoded_response_stream_t responses = http::decode_stream< http::response_header >( response_text );

		std::vector< response_t > result;

		for ( http::decoded_response_stream_t::const_iterator r = responses.begin(); r != responses.end(); ++r )
		{
			const json::array messages = json::parse( r->second.begin(), r->second.end() ).upcast< json::array >();
			result.push_back( std::make_pair( r->first, messages ) );
		}

		return result;
	}

	/*!
	 * Takes the simulated client input, records the response and extracts the bayeux messages from the
	 * http responses.
	 */
	std::vector< response_t > bayeux_session( const server::test::read_plan& input, test_context& context )
	{
		socket_t	socket( context.queue, input );

		boost::shared_ptr< connection_t > connection( new connection_t( socket, context.trait ) );
		connection->start();

		tools::run( context.queue );

		return extract_responses( socket.bin_output() );
	}

	std::vector< response_t > bayeux_session( const server::test::read_plan& input )
	{
		test_context	context;
		return bayeux_session( input, context );
	}
}

/**
 * @test simulate a handshake to the server
 */
BOOST_AUTO_TEST_CASE( bayeux_handshake )
{
	std::vector< response_t > response = bayeux_session(
		server::test::read_plan()
			<< bayeux_msg(
				"{ 'channel' : '/meta/handshake',"
				"  'version' : '1.0.0',"
				"  'supportedConnectionTypes' : ['long-polling', 'callback-polling', 'iframe'] }" )
			<< server::test::disconnect_read() );

	BOOST_REQUIRE_EQUAL( 1u, response.size() );

	const json::array handshake_response_container  = response.front().second;
	BOOST_REQUIRE_EQUAL( 1u, handshake_response_container.length() );

	const json::object handshake_response = handshake_response_container.at( 0 ).upcast< json::object >();

	BOOST_CHECK_EQUAL( handshake_response.at( json::string( "channel" ) ), json::string( "/meta/handshake" ) );
	BOOST_CHECK_EQUAL( handshake_response.at( json::string( "successful" ) ), json::true_val() );
	BOOST_CHECK_EQUAL( handshake_response.at( json::string( "clientId" ) ), json::string( "192.168.210.1:9999/0" ) );
	BOOST_CHECK_NE( handshake_response.at( json::string( "version" ) ), json::null() );
	BOOST_CHECK_NE( handshake_response.at( json::string( "supportedConnectionTypes" ) ), json::null() );
}

static bool success_handshake_and_connect(
		const json::array& handshake_response_container,
		const json::array& connect_response_container )
{
	BOOST_REQUIRE_EQUAL( 1u, handshake_response_container.length() );
	BOOST_REQUIRE_EQUAL( 1u, connect_response_container.length() );

	const json::object handshake_response = handshake_response_container.at( 0 ).upcast< json::object >();
	const json::object connect_response   = connect_response_container.at( 0 ).upcast< json::object >();

	BOOST_CHECK_EQUAL( handshake_response.at( json::string( "channel" ) ), json::string( "/meta/handshake" ) );
	BOOST_CHECK_EQUAL( handshake_response.at( json::string( "successful" ) ), json::true_val() );
	BOOST_CHECK_EQUAL( handshake_response.at( json::string( "clientId" ) ), json::string( "192.168.210.1:9999/0" ) );
	BOOST_CHECK_NE( handshake_response.at( json::string( "version" ) ), json::null() );
	BOOST_CHECK_NE( handshake_response.at( json::string( "supportedConnectionTypes" ) ), json::null() );

	BOOST_CHECK_EQUAL( connect_response.at( json::string( "channel" ) ), json::string( "/meta/connect" ) );
	BOOST_CHECK_EQUAL( connect_response.at( json::string( "successful" ) ), json::true_val() );
	BOOST_CHECK_EQUAL( connect_response.at( json::string( "clientId" ) ), json::string( "192.168.210.1:9999/0" ) );

	return true;
}

/**
 * @test simply connect after handshake must result in an established connection
 */
BOOST_AUTO_TEST_CASE( handshake_and_connect )
{
	std::vector< response_t > response = bayeux_session(
		server::test::read_plan()
			<< bayeux_msg(
				"{ 'channel' : '/meta/handshake',"
				"  'version' : '1.0.0',"
				"  'supportedConnectionTypes' : ['long-polling', 'callback-polling'] }" )
			<< bayeux_msg(
				"{ 'channel' : '/meta/connect',"
				"  'clientId' : '192.168.210.1:9999/0',"
				"  'connectionType' : 'long-polling' }" )
			<< server::test::disconnect_read() );

	BOOST_REQUIRE_EQUAL( 2u, response.size() );

	const json::array handshake_response_container  = response.front().second;
	const json::array connect_response_container    = response.back().second;

	BOOST_CHECK( success_handshake_and_connect( handshake_response_container, connect_response_container ) );
}

/**
 * @brief same test, as above, but this time the handshake and requests are made in two different requests
 */
BOOST_AUTO_TEST_CASE( handshake_and_connect_with_two_requests )
{
	test_context context;

	const std::vector< response_t > first_response = bayeux_session(
		server::test::read_plan()
			<< bayeux_msg(
				"{ 'channel' : '/meta/handshake',"
				"  'version' : '1.0.0',"
				"  'supportedConnectionTypes' : ['long-polling', 'callback-polling'] }" )
			<< server::test::disconnect_read(),
		context );

	BOOST_REQUIRE_EQUAL( 1u, first_response.size() );

	const std::vector< response_t > second_response = bayeux_session(
		server::test::read_plan()
			<< bayeux_msg(
				"{ 'channel' : '/meta/connect',"
				"  'clientId' : '192.168.210.1:9999/0',"
				"  'connectionType' : 'long-polling' }" )
			<< server::test::disconnect_read(),
		context );

	BOOST_REQUIRE_EQUAL( 1u, second_response.size() );

	const json::array handshake_response_container  = first_response.front().second;
	const json::array connect_response_container    = second_response.front().second;
	BOOST_CHECK( success_handshake_and_connect( handshake_response_container, connect_response_container ) );
}

// checks that the response contains a single response to a connect response, that indicated failure
// and returns that response.
static json::object failed_connect( const std::vector< response_t >& response )
{
	BOOST_REQUIRE_EQUAL( 1u, response.size() );

	const json::array response_container = response.front().second;
	BOOST_REQUIRE_EQUAL( 1u, response_container.length() );

	const json::object connect_response = response_container.at( 0 ).upcast< json::object >();

	BOOST_CHECK_EQUAL( connect_response.at( json::string( "channel" ) ), json::string( "/meta/connect" ) );
	BOOST_CHECK_EQUAL( connect_response.at( json::string( "successful" ) ), json::false_val() );

	return connect_response;
}

/**
 * @test connect without valid client id must result in connection failure
 */
BOOST_AUTO_TEST_CASE( bayeux_connection_with_invalid_id_must_fail )
{
	const std::vector< response_t > response = bayeux_session(
		server::test::read_plan()
			<< bayeux_msg(
				"{ 'channel' : '/meta/connect',"
				"  'clientId' : '192.168.210.1:9999/42',"
				"  'connectionType' : 'long-polling' }" )
			<< server::test::disconnect_read() );

	BOOST_CHECK_EQUAL( failed_connect( response ).at( json::string( "clientId" ) ),
			json::string( "192.168.210.1:9999/42" ) );
}

/**
 * @test connect without valid client id must result in connection failure
 */
BOOST_AUTO_TEST_CASE( bayeux_connection_with_unsupported_connection_type_must_fail )
{
	test_context context;

	bayeux_session(
		server::test::read_plan()
			<< bayeux_msg(
				"{ 'channel' : '/meta/handshake',"
				"  'version' : '1.0.0',"
				"  'supportedConnectionTypes' : ['long-polling', 'callback-polling'] }" )
			<< server::test::disconnect_read(),
		context );

	const std::vector< response_t > response = bayeux_session(
		server::test::read_plan()
			<< bayeux_msg(
				"{ 'channel' : '/meta/connect',"
				"  'clientId' : '192.168.210.1:9999/0',"
				"  'connectionType' : 'long-fooling' }" )
			<< server::test::disconnect_read(),
		context );

	BOOST_CHECK_EQUAL( failed_connect( response ).at( json::string( "clientId" ) ),
			json::string( "192.168.210.1:9999/0" ) );
}

/**
 * @test simple handshake subscribe and connect
 */
BOOST_AUTO_TEST_CASE( bayeux_simple_handshake_subscribe_connect )
{
	test_context context;

	const std::vector< response_t > response = bayeux_session(
		server::test::read_plan()
			<< bayeux_msg(
				"{ 'channel' : '/meta/handshake',"
				"  'version' : '1.0.0',"
				"  'supportedConnectionTypes' : ['long-polling', 'callback-polling'] }" )
			<< bayeux_msg(
				"{ 'channel' : '/meta/subscribe',"
				"  'version' : '1.0.0',"
				"  'subscription' : '/foo/bar' }" )
			<< bayeux_msg(
				"{ 'channel' : '/meta/connect',"
				"  'clientId' : '192.168.210.1:9999/0',"
				"  'connectionType' : 'long-pooling' }" )
			<< server::test::disconnect_read(),
		context );

	BOOST_REQUIRE_EQUAL( 3u, response.size() );

	const json::array handshake_response_container  = response[ 0 ].second;
	const json::array subscribe_response_container  = response[ 1 ].second;
	const json::array connect_response_container    = response[ 2 ].second;

	BOOST_CHECK( success_handshake_and_connect( handshake_response_container, connect_response_container ) );
}

/**
 * @test subscription to a list of channels
 */
BOOST_AUTO_TEST_CASE( bayeux_subscribe_to_a_bunch_of_channels )
{
}

/**
 * @test check, that only configured connection types are considered
 */
BOOST_AUTO_TEST_CASE( bayeux_only_supported_connection_types )
{
}

/**
 * @test between a handshake and a connect, the
 */
BOOST_AUTO_TEST_CASE( handshake_break_connect_subscribe_disconnect )
{
	// meldungen
	//
}

/**
 * @test what should happen, if none of the enumerated connection type is supported by the server
 */
BOOST_AUTO_TEST_CASE( no_supported_connection_type )
{
}

/**
 * @test a http proxy could use one http connection to connect more than one client to the bayeux server
 */
BOOST_AUTO_TEST_CASE( more_than_one_session_in_a_single_connection )
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
