// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#define BOOST_TEST_MAIN

#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "bayeux/bayeux.h"
#include "bayeux/configuration.h"
#include "bayeux/node_channel.h"
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
#include "server/test_timer.h"
#include "server/traits.h"
#include "tools/asstring.h"
#include "tools/io_service.h"

namespace
{
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
	      : connector_( trait.queue(), trait.data(), session_generator_, trait.config() )
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

	    bayeux::connector< server::test::timer>	connector_;
	    server::test::session_generator	        session_generator_;
	};

	class trait_data
	{
	public:
		trait_data( boost::asio::io_service& queue, pubsub::root& data, const bayeux::configuration& config )
			: queue_( queue )
		    , data_( data )
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

		boost::asio::io_service& queue() const
		{
		    return queue_;
		}

		const bayeux::configuration& config() const
		{
			return config_;
		}
	private:
		boost::asio::io_service&    queue_;
		pubsub::root& 			    data_;
		bayeux::configuration 	    config_;
	};

    typedef server::test::timer         timer_t;
	typedef server::test::socket< const char*, timer_t > socket_t;
	typedef server::null_event_logger	event_logger_t;
//	typedef server::stream_event_log	event_logger_t;
	typedef server::stream_error_log	error_logger_t;

	class trait_t :
		public trait_data,
		public server::connection_traits< socket_t, timer_t, response_factory, event_logger_t, error_logger_t >
	{
	public:
		typedef server::connection_traits< socket_t, timer_t, response_factory, event_logger_t, error_logger_t > base_t;

	    trait_t( boost::asio::io_service& queue, pubsub::root& data,
	                const bayeux::configuration& config = bayeux::configuration() )
			: trait_data( queue, data, config )
			, base_t( *this )
		{
		}
	};

    /*
     * everything that is needed for a complete test
     */
	struct test_context
	{
		boost::asio::io_service	queue;
		pubsub::test::adapter	adapter;
		pubsub::root			data;
		trait_t					trait;

		test_context()
			: queue()
			, adapter()
			, data( queue, adapter, pubsub::configuration() )
			, trait( queue, data )
		{
		    server::test::reset_time();
		}

		explicit test_context( const pubsub::configuration& config )
            : queue()
            , adapter()
            , data( queue, adapter, config )
            , trait( queue, data )
        {
            server::test::reset_time();
        }

        test_context( const pubsub::configuration& config, const bayeux::configuration& bayeux_config )
            : queue()
            , adapter()
            , data( queue, adapter, config )
            , trait( queue, data, bayeux_config )
        {
            server::test::reset_time();
        }

	};

	typedef server::connection< trait_t >                           connection_t;

	struct response_t
	{
	    boost::shared_ptr< http::response_header >  first;
	    json::array                                 second;
	    boost::posix_time::ptime                    received;
	};

	class stream_decoder
	{
	public:
	    stream_decoder()
	        : idle_( true )
	    {}

	    void operator()( boost::asio::const_buffer data )
	    {
	        for ( ; buffer_size( data ); data = feed_data( data ) )
	            ;
	    }

	    std::vector< response_t > result() const
        {
	        if ( !idle_ )
	            throw std::runtime_error( "incomplete http message" );

	        return result_;
        }
	private:
	    boost::asio::const_buffer feed_data( const boost::asio::const_buffer& data )
	    {
	        const std::pair< bool, std::size_t > remaining = decoder_.feed_data( data );
            idle_ = remaining.first && remaining.second == 0;

	        if ( remaining.first )
	        {
	            std::pair< boost::shared_ptr< http::response_header >, std::vector< char > > msg =
	                decoder_.last_message();

	            const response_t new_response = {
	                msg.first,
	                json::parse( msg.second.begin(), msg.second.end() ).upcast< json::array >(),
	                server::test::current_time()
	            };

	            result_.push_back( new_response );
	        }

	        return data + ( buffer_size( data ) - remaining.second );
	    }

	    http::stream_decoder< http::response_header >   decoder_;
	    std::vector< response_t >                       result_;
	    bool                                            idle_;
	};

	/*
	 * splits the response byte stream in the http and the bayeux part
	 */
	std::vector< response_t > extract_responses( const std::vector< char >& response_text )
	{
		const http::decoded_response_stream_t responses = http::decode_stream< http::response_header >( response_text );

		std::vector< response_t > result;

		for ( http::decoded_response_stream_t::const_iterator r = responses.begin(); r != responses.end(); ++r )
		{
		    const response_t response =
		    {
		        r->first,
		        json::parse( r->second.begin(), r->second.end() ).upcast< json::array >(),
		        server::test::current_time()
		    };
			result.push_back( response );
		}

		return result;
	}

	/*
	 * Takes the simulated client input, records the response and extracts the bayeux messages from the
	 * http responses. At the beginning of the session, the time is set to "1970-01-01 00:00:00"
	 */
	std::vector< response_t > bayeux_session( const server::test::read_plan& input,
	    const server::test::write_plan& output, test_context& context )
	{
        stream_decoder  decoder;
        socket_t	    socket( context.queue, input, output );
        socket.write_callback( boost::bind< void >( boost::ref( decoder ), _1 ) );

		boost::shared_ptr< connection_t > connection( new connection_t( socket, context.trait ) );
		connection->start();

		try
		{
		    tools::run( context.queue );
		}
		catch (...)
		{
		    std::cerr << "error running bayeux_session" << std::endl;
		    BOOST_REQUIRE( false );
		}

		return decoder.result();
	}

    std::vector< response_t > bayeux_session( const server::test::read_plan& input, test_context& context )
    {
        return bayeux_session( input, server::test::write_plan(), context );
    }

    std::vector< response_t > bayeux_session( const server::test::read_plan& input )
	{
		test_context	context;
		return bayeux_session( input, server::test::write_plan(), context );
	}

	/*
	 * extracts the bayeux messages from the given list of reponses
	 */
	json::array bayeux_messages( const std::vector< response_t >& http_response )
	{
	    json::array result;

	    for ( std::vector< response_t >::const_iterator m = http_response.begin(); m != http_response.end(); ++m )
	        result += m->second;

	    return result;
	}

	static void post_to_io_queue( boost::asio::io_service& queue, const boost::function< void() >& f )
	{
	    queue.post( f );
	}

	boost::function< void() > update_node( test_context& context, const char* channel_name, const json::value& data,
	    const json::value* id = 0 )
	{
	    json::object message;
	    message.add( json::string( "data" ), data );
	    if ( id )
	        message.add( json::string( "id" ), *id );

	    const boost::function< void() >func = boost::bind(
	            &pubsub::root::update_node,
	            boost::ref( context.data ),
	            bayeux::node_name_from_channel( channel_name ),
	            message );

	    return boost::bind( &post_to_io_queue, boost::ref( context.queue ), func );
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
 *       A passed id field in the request have to appear in the response
 */
BOOST_AUTO_TEST_CASE( bayeux_connection_with_invalid_id_must_fail_with_custom_id )
{
    const std::vector< response_t > response = bayeux_session(
        server::test::read_plan()
            << bayeux_msg(
                "{ 'channel' : '/meta/connect',"
                "  'clientId' : '192.168.210.1:9999/42',"
                "  'connectionType' : 'long-polling',"
                "   'id' : 'test' }" )
            << server::test::disconnect_read() );

    const json::object connect_response = failed_connect( response );
    BOOST_CHECK_EQUAL( connect_response.at( json::string( "clientId" ) ), json::string( "192.168.210.1:9999/42" ) );
    BOOST_CHECK_EQUAL( connect_response.at( json::string( "id" ) ), json::string( "test" ) );
}

/**
 * @test connect with unsupported connection type must fail.
 */
BOOST_AUTO_TEST_CASE( bayeux_connection_with_unsupported_connection_type_must_fail )
{
	test_context context;

	const std::vector< response_t > response = bayeux_session(
		server::test::read_plan()
            << bayeux_msg(
                "{ "
                "   'channel' : '/meta/handshake',"
                "   'version' : '1.0.0',"
                "   'supportedConnectionTypes' : ['long-polling', 'callback-polling'] "
                "}" )
			<< bayeux_msg(
				"{ "
				"   'channel' : '/meta/connect',"
				"   'clientId' : '192.168.210.1:9999/0',"
				"   'connectionType' : 'long-fooling' "
				"}" )
			<< server::test::disconnect_read(),
		context );

	BOOST_REQUIRE_EQUAL( 2u, response.size() );
	BOOST_CHECK_EQUAL(
	    response[ 1u ].second,
	    json::parse_single_quoted(
	        "[{"
	        "   'channel'    : '/meta/connect',"
	        "   'clientId'   : '192.168.210.1:9999/0',"
	        "   'successful' : false,"
	        "   'error'      : 'unsupported connection type'"
	        "}]") );
}

/**
 * @test connect with unsupported connection type must fail.
 *       This time with an id in the request message an both messages are send with a single http request.
 */
BOOST_AUTO_TEST_CASE( bayeux_connection_with_unsupported_connection_type_must_fail_with_id_and_single_http_request )
{
    test_context context;

    const json::array response = bayeux_messages( bayeux_session(
        server::test::read_plan()
            << bayeux_msg(
                "[{ "
                "   'channel' : '/meta/handshake',"
                "   'version' : '1.0.0',"
                "   'supportedConnectionTypes' : ['long-polling', 'callback-polling'] "
                "},{ "
                "   'channel' : '/meta/connect',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'connectionType' : 'long-fooling',"
                "   'id' : 'foo'"
                "}]" )
            << server::test::disconnect_read(),
        context ) );

    BOOST_REQUIRE_EQUAL( 2u, response.length() );
    BOOST_CHECK_EQUAL(
        response.at( 1u ),
        json::parse_single_quoted(
            "{"
            "   'channel'    : '/meta/connect',"
            "   'clientId'   : '192.168.210.1:9999/0',"
            "   'successful' : false,"
            "   'error'      : 'unsupported connection type',"
            "   'id'         : 'foo'"
            "}") );
}

/**
 * @test simple handshake subscribe and connect
 */
BOOST_AUTO_TEST_CASE( bayeux_simple_handshake_subscribe_connect )
{
    test_context context( pubsub::configurator().authorization_not_required() );

	context.adapter.answer_validation_request( bayeux::node_name_from_channel( "/foo/bar" ), true );
	context.adapter.answer_initialization_request( bayeux::node_name_from_channel( "/foo/bar" ), json::null() );

	const json::array response = bayeux_messages( bayeux_session(
		server::test::read_plan()
			<< bayeux_msg(
				"{ 'channel' : '/meta/handshake',"
				"  'version' : '1.0.0',"
				"  'supportedConnectionTypes' : ['long-polling', 'callback-polling'],"
				"  'id'      : 'connect_id' }" )
			<< bayeux_msg(
				"{ 'channel' : '/meta/subscribe',"
			    "  'clientId' : '192.168.210.1:9999/0',"
				"  'subscription' : '/foo/bar' }" )
			<< bayeux_msg(
				"{ 'channel' : '/meta/connect',"
				"  'clientId' : '192.168.210.1:9999/0',"
				"  'connectionType' : 'long-polling' }" )
			<< server::test::disconnect_read(),
		context ) );

	BOOST_REQUIRE_EQUAL( 3u, response.length() );

	BOOST_CHECK_EQUAL( response, json::parse_single_quoted(
	    "["
	    "   {"
	    "       'channel'       : '/meta/handshake',"
	    "       'version'       : '1.0',"
	    "       'clientId'      : '192.168.210.1:9999/0',"
	    "       'successful'    : true,"
	    "       'supportedConnectionTypes': ['long-polling'],"
	    "       'id'            : 'connect_id'"
	    "   },"
	    "   {"
	    "       'channel'       : '/meta/subscribe',"
	    "       'clientId'      : '192.168.210.1:9999/0',"
	    "       'successful'    : true,"
	    "       'subscription'  : '/foo/bar'"
	    "   },"
	    "   {"
	    "       'channel'       : '/meta/connect',"
        "       'clientId'      : '192.168.210.1:9999/0',"
        "       'successful'    : true"
	    "   }"
	    "]" ) );
}

/**
 * @test test that a connect blocks if nothing is there to be send
 *
 * The test is based on the current implementation, where a subscription will not directly respond.
 * The first connect will collect the subscribe respond from the first message, the second connect will
 * then block until the update is received.
 */
BOOST_AUTO_TEST_CASE( bayeux_connect_blocks_until_an_event_happens )
{
    test_context context( pubsub::configurator().authorization_not_required() );

    context.adapter.answer_validation_request( bayeux::node_name_from_channel( "/foo/bar" ), true );
    context.adapter.answer_initialization_request( bayeux::node_name_from_channel( "/foo/bar" ), json::null() );

    json::array response = bayeux_messages( bayeux_session(
        server::test::read_plan()
            << bayeux_msg(
                "[{ "
                "   'channel' : '/meta/handshake',"
                "   'version' : '1.0.0',"
                "   'supportedConnectionTypes' : ['long-polling', 'callback-polling']"
                "},{ "
                "   'channel' : '/meta/subscribe',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'subscription' : '/foo/bar' "
                "}]" )
            << bayeux_msg(
                "{ "
                "   'channel' : '/meta/connect',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'connectionType' : 'long-polling'"
                "}" )
            << bayeux_msg(
                "{ "
                "   'channel' : '/meta/connect',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'connectionType' : 'long-polling',"
                "   'id' : 'second_connect'"
                "}" )
             << update_node( context, "/foo/bar", json::number( 42 ) )
             << server::test::disconnect_read(),
        context ) );

    BOOST_REQUIRE( !response.empty() );
    // the /meta/handshake response is already tested.
    response.erase( 0, 1 );

    BOOST_CHECK_EQUAL( response,
        json::parse_single_quoted(
            "["
            "   {"
            "       'channel'       : '/meta/subscribe',"
            "       'clientId'      : '192.168.210.1:9999/0',"
            "       'subscription'  : '/foo/bar',"
            "       'successful'    : true"
            "   },"
            "   {"
            "       'channel'   : '/meta/connect',"
            "       'clientId'  : '192.168.210.1:9999/0',"
            "       'successful': true"
            "   },"
            "   {"
            "       'channel'   : '/foo/bar',"
            "       'data'      : 42"
            "   },"
            "   {"
            "       'channel'   : '/meta/connect',"
            "       'clientId'  : '192.168.210.1:9999/0',"
            "       'successful': true,"
            "       'id'        : 'second_connect'"
            "   }"
            "]"
        ) );
}

/**
 * @test what should happen, if the connection to the client get closed (writing part) while the response is blocked?
 */
BOOST_AUTO_TEST_CASE( http_connection_get_closed_while_response_is_waiting )
{
    test_context context( pubsub::configurator().authorization_not_required() );

    context.adapter.answer_validation_request( bayeux::node_name_from_channel( "/foo/bar" ), true );
    context.adapter.answer_initialization_request( bayeux::node_name_from_channel( "/foo/bar" ), json::null() );

    bayeux_session(
        server::test::read_plan()
            << bayeux_msg(
                "[{ "
                "   'channel' : '/meta/handshake',"
                "   'version' : '1.0.0',"
                "   'supportedConnectionTypes' : ['long-polling', 'callback-polling']"
                "},{ "
                "   'channel' : '/meta/subscribe',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'subscription' : '/foo/bar' "
                "}]" )
            << bayeux_msg(
                "{ "
                "   'channel' : '/meta/connect',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'connectionType' : 'long-polling'"
                "}" )
             << server::test::disconnect_read(),
        context );

    socket_t    socket(
        context.queue,
        server::test::read_plan()
            << bayeux_msg(
                "{ "
                "   'channel' : '/meta/connect',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'connectionType' : 'long-polling'"
                "}" )
            << update_node( context, "/foo/bar", json::number( 42 ) )
            << server::test::disconnect_read(),
        server::test::write_plan()
            << server::test::write( 10 )
            << make_error_code( boost::asio::error::connection_reset ) );

    boost::shared_ptr< connection_t > connection( new connection_t( socket, context.trait ) );
    connection->start();

    tools::run( context.queue );

    // now the session should still be available
    bayeux::session* session =
        context.trait.connector_.find_session( json::string( "192.168.210.1:9999/0" ) );

    BOOST_CHECK( session );
    context.trait.connector_.idle_session( session );
}

/**
 * @test check, that only configured connection types are considered
 */
BOOST_AUTO_TEST_CASE( bayeux_only_supported_connection_types )
{
    /// @todo should be implemented, when a second connection type is implemented
}
/**
 * @test currently the response is to disconnect when the body contains errors.
 *       In a later version there should be a HTTP error response, if the body was completely received.
 */
BOOST_AUTO_TEST_CASE( incomplete_bayeux_request_should_result_in_http_error_response )
{
    const std::vector< response_t > response = bayeux_session(
        server::test::read_plan()
            << bayeux_msg("[{]") // a somehow broken message
            );

    BOOST_REQUIRE_EQUAL( 0u, response.size() );
}

namespace
{
    unsigned count_fields( const json::array& list, const json::string& field_name, const json::value& value )
    {
        unsigned result = 0;
        for ( std::size_t i = 0, length = list.length(); i != length; ++i )
        {
            const std::pair< bool, json::object > element = list.at( i ).try_cast< json::object >();

            if ( element.first )
            {
                const json::value* const value_found = element.second.find( field_name );
                if ( value_found && *value_found == value )
                    ++result;
            }
        }

        return result;
    }

    bool occurences_in_range( const std::vector< response_t >& response, const char* field, const char* value,
        unsigned min, unsigned max )
    {
        const json::string field_name( field );
        const json::value field_value = json::parse_single_quoted( value );

        bool result = true;
        for ( std::vector< response_t >::const_iterator i = response.begin(); i != response.end() && result; ++i )
        {
            const unsigned count = count_fields( i->second, field_name, field_value );
            result = count >= min && count <= max;
        }

        return result;
    }

    bool contains_at_least_once( const std::vector< response_t >& response, const char* field, const char* value )
    {
        return occurences_in_range( response, field, value, 1u, 100000000u );
    }

    bool contains_not( const std::vector< response_t >& response, const char* field, const char* value )
    {
        return occurences_in_range( response, field, value, 0, 0 );
    }
}

/**
 * @test a http proxy could use one http connection to connect more than one client to the bayeux server
 */
BOOST_AUTO_TEST_CASE( more_than_one_session_in_a_single_connection )
{
    test_context context( pubsub::configurator().authorization_not_required() );

    context.adapter.answer_validation_request( bayeux::node_name_from_channel( "/foo/bar" ), true );
    context.adapter.answer_initialization_request( bayeux::node_name_from_channel( "/foo/bar" ), json::null() );

    const std::vector< response_t > response = bayeux_session(
        server::test::read_plan()
            << bayeux_msg(
                "{"
                "   'channel' : '/meta/handshake',"
                "   'version' : '1.0.0',"
                "   'supportedConnectionTypes' : ['long-polling', 'callback-polling'],"
                "   'id'      : 'id_first_handshake'"
                "}" )
            << bayeux_msg(
                "[{"
                "   'channel' : '/meta/subscribe',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'subscription' : '/foo/bar' "
                "},{ "
                "   'channel' : '/meta/connect',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'connectionType' : 'long-polling' "
                "}]" )
            << bayeux_msg(
                "[{ "
                "   'channel' : '/meta/handshake',"
                "   'version' : '1.0.0',"
                "   'supportedConnectionTypes' : ['long-polling', 'callback-polling'],"
                "   'id'      : 'id_second_handshake'"
                "}]")
            << bayeux_msg(
                "[{ "
                "   'channel'      : '/meta/subscribe',"
                "   'clientId'     : '192.168.210.1:9999/1',"
                "   'subscription' : '/foo/bar' "
                "},{ "
                "   'channel'      : '/meta/connect',"
                "   'clientId'     : '192.168.210.1:9999/1',"
                "   'connectionType' : 'long-polling' "
                "}]" )
            << bayeux_msg(
                "[{ "
                "   'channel' : '/meta/connect',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'connectionType' : 'long-polling'"
                "}]" )
            << bayeux_msg(
                "[{ "
                "   'channel'  : '/meta/connect',"
                "   'clientId' : '192.168.210.1:9999/1',"
                "   'connectionType' : 'long-polling'"
                "}]" )
             << update_node( context, "/foo/bar", json::number( 42 ) )
             << server::test::disconnect_read(),
        context );

    BOOST_REQUIRE_EQUAL( 6u, response.size() );
    std::vector< response_t > response_first_client;
    std::vector< response_t > response_second_client;
    response_first_client.push_back( response[ 0 ] );
    response_first_client.push_back( response[ 1 ] );
    response_second_client.push_back( response[ 2 ] );
    response_second_client.push_back( response[ 3 ] );
    response_first_client.push_back( response[ 4 ] );
    response_second_client.push_back( response[ 5 ] );

    // now in the response to the first session in ever response, the session id must be mentioned and the
    // session id from the second session must not be contained
    BOOST_CHECK( contains_at_least_once( response_first_client, "clientId", "'192.168.210.1:9999/0'" ) );
    BOOST_CHECK( contains_not( response_first_client, "clientId", "'192.168.210.1:9999/1'" ) );
    BOOST_CHECK( contains_at_least_once( response_second_client, "clientId", "'192.168.210.1:9999/1'" ) );
    BOOST_CHECK( contains_not( response_second_client, "clientId", "'192.168.210.1:9999/0'" ) );

    // getting the session id for the first session
    BOOST_REQUIRE_LE( 1u, response.size() );
    BOOST_CHECK_EQUAL( response[ 0 ].second,
        json::parse_single_quoted(
            "["
            "   {"
            "       'channel'       : '/meta/handshake',"
            "       'version'       : '1.0',"
            "       'supportedConnectionTypes' : ['long-polling'],"
            "       'clientId'      : '192.168.210.1:9999/0',"
            "       'successful'    : true,"
            "       'id'            : 'id_first_handshake'"
            "   }"
            "]"
        ) );

    // getting the session id for the second session
    BOOST_REQUIRE_LE( 3u, response.size() );
    BOOST_CHECK_EQUAL( response[ 2u ].second,
        json::parse_single_quoted(
            "["
            "   {"
            "       'channel'       : '/meta/handshake',"
            "       'version'       : '1.0',"
            "       'supportedConnectionTypes' : ['long-polling'],"
            "       'clientId'      : '192.168.210.1:9999/1',"
            "       'successful'    : true,"
            "       'id'            : 'id_second_handshake'"
            "   }"
            "]"
        ) );

}

/**
 * @brief hurry a listening connection, if a normal http request is pipelined
 *
 * The second http request should block because it contains a connect.
 */
BOOST_AUTO_TEST_CASE( hurry_bayeux_connection_if_request_is_pipelined )
{
    test_context context( pubsub::configurator().authorization_not_required() );

    const std::vector< response_t > response = bayeux_session(
        server::test::read_plan()
            << bayeux_msg(
                "{"
                "   'channel' : '/meta/handshake',"
                "   'version' : '1.0.0',"
                "   'supportedConnectionTypes' : ['long-polling', 'callback-polling'],"
                "   'id'      : 'id_first_handshake'"
                "}" )
            << bayeux_msg(
                "[{"
                "   'channel' : '/meta/subscribe',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'subscription' : '/foo/bar' "
                "},{ "
                "   'channel' : '/meta/connect',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'connectionType' : 'long-polling' "
                "}]" )
            << bayeux_msg(
                "[{"
                "   'channel' : '/meta/subscribe',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'subscription' : '/foo/chu' "
                "}]" )
            << server::test::disconnect_read(),
            context );

    BOOST_REQUIRE_EQUAL( response.size(), 3u );
    BOOST_REQUIRE_EQUAL(
        response[ 1 ].second,
        json::parse_single_quoted(
            "["
            "   {"
            "       'channel'       : '/meta/connect',"
            "       'clientId'      : '192.168.210.1:9999/0',"
            "       'successful'    : true"
            "   }"
            "]" ) );

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

/**
 * @test a connect that is not the last bayeux message, should not block
 */
BOOST_AUTO_TEST_CASE( single_http_request_with_connect_not_beeing_the_last_element )
{
}

/**
 * @test a connect should not block for ever, but instead for the configured poll timeout
 */
BOOST_AUTO_TEST_CASE( long_poll_time_out_test )
{
    const boost::posix_time::seconds timeout( 100 );

    test_context context(
        pubsub::configurator().authorization_not_required(),
        bayeux::configuration().session_timeout( timeout ) );

    const boost::posix_time::ptime   start_time = server::test::current_time();

    context.adapter.answer_validation_request( bayeux::node_name_from_channel( "/foo/bar" ), true );
    context.adapter.answer_initialization_request( bayeux::node_name_from_channel( "/foo/bar" ), json::null() );

    const std::vector< response_t > response = bayeux_session(
        server::test::read_plan()
            << bayeux_msg(
                "{"
                "   'channel' : '/meta/handshake',"
                "   'version' : '1.0.0',"
                "   'supportedConnectionTypes' : ['long-polling', 'callback-polling'],"
                "   'id'      : 'id_first_handshake'"
                "}" )
            << bayeux_msg(
                "[{"
                "   'channel' : '/meta/subscribe',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'subscription' : '/foo/bar' "
                "},{ "
                "   'channel' : '/meta/connect',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'connectionType' : 'long-polling' "
                "}]" )
           << bayeux_msg(
               "{"
               "  'channel' : '/meta/connect',"
               "   'clientId' : '192.168.210.1:9999/0',"
               "   'connectionType' : 'long-polling' "
               "}" )
           << server::test::delay( timeout + timeout )
           << server::test::disconnect_read(),
       server::test::write_plan(),
       context );

    BOOST_REQUIRE_EQUAL( 3u, response.size() );

    BOOST_CHECK_EQUAL( response[ 2u ].first->code(), http::http_ok );
    BOOST_CHECK( response[ 2u ].second.empty() );
    BOOST_CHECK_EQUAL( response[ 2u ].received - start_time, timeout );
}

