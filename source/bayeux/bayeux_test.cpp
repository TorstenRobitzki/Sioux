// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#define BOOST_TEST_MAIN

#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "bayeux/bayeux.h"
#include "bayeux/node_channel.h"
#include "bayeux/test_tools.h"
#include "server/connection.h"
#include "tools/asstring.h"
#include "tools/io_service.h"

/**
 * @test simulate a handshake to the server
 */
BOOST_AUTO_TEST_CASE( bayeux_handshake )
{
	std::vector< bayeux::test::response_t > response = bayeux::test::bayeux_session(
		server::test::read_plan()
			<< bayeux::test::msg(
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
static json::object failed_connect( const std::vector< bayeux::test::response_t >& response )
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
	const std::vector< bayeux::test::response_t > response = bayeux::test::bayeux_session(
		server::test::read_plan()
			<< bayeux::test::msg(
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
    const std::vector< bayeux::test::response_t > response = bayeux::test::bayeux_session(
        server::test::read_plan()
            << bayeux::test::msg(
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
    bayeux::test::context context;

	const std::vector< bayeux::test::response_t > response = bayeux::test::bayeux_session(
		server::test::read_plan()
            << bayeux::test::msg(
                "{ "
                "   'channel' : '/meta/handshake',"
                "   'version' : '1.0.0',"
                "   'supportedConnectionTypes' : ['long-polling', 'callback-polling'] "
                "}" )
			<< bayeux::test::msg(
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
    bayeux::test::context context;

    const json::array response = bayeux_messages( bayeux::test::bayeux_session(
        server::test::read_plan()
            << bayeux::test::msg(
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
    bayeux::test::context context( pubsub::configurator().authorization_not_required() );

	context.adapter.answer_validation_request( bayeux::node_name_from_channel( "/foo/bar" ), true );
	context.adapter.answer_initialization_request( bayeux::node_name_from_channel( "/foo/bar" ), json::null() );

	const json::array response = bayeux::test::bayeux_messages( bayeux::test::bayeux_session(
		server::test::read_plan()
			<< bayeux::test::msg(
				"{ 'channel' : '/meta/handshake',"
				"  'version' : '1.0.0',"
				"  'supportedConnectionTypes' : ['long-polling', 'callback-polling'],"
				"  'id'      : 'connect_id' }" )
			<< bayeux::test::msg(
				"{ 'channel' : '/meta/subscribe',"
			    "  'clientId' : '192.168.210.1:9999/0',"
				"  'subscription' : '/foo/bar' }" )
			<< bayeux::test::msg(
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
 * @test subscribe without 'subscription' must fail
 */
BOOST_AUTO_TEST_CASE( subscribe_without_subject )
{
    bayeux::test::context context;

    const std::vector< bayeux::test::response_t > response = bayeux::test::bayeux_session(
        server::test::read_plan()
            << bayeux::test::msg(
                "{ 'channel' : '/meta/handshake',"
                "  'version' : '1.0.0',"
                "  'supportedConnectionTypes' : ['long-polling', 'callback-polling'],"
                "  'id'      : 'connect_id' }" )
            << bayeux::test::msg(
                "{ 'channel' : '/meta/subscribe',"
                "  'clientId' : '192.168.210.1:9999/0' }" )
            << server::test::disconnect_read(),
        context );

    BOOST_REQUIRE_EQUAL( 1u, response.size() );
    BOOST_CHECK_EQUAL( response[ 0 ].second, json::parse_single_quoted(
        "["
        "   {"
        "       'channel'       : '/meta/handshake',"
        "       'version'       : '1.0',"
        "       'clientId'      : '192.168.210.1:9999/0',"
        "       'successful'    : true,"
        "       'supportedConnectionTypes': ['long-polling'],"
        "       'id'            : 'connect_id'"
        "   }"
        "]" ) );
}

/**
 * @test subscribe without client-id must fail
 */
BOOST_AUTO_TEST_CASE( subscribe_without_client_id )
{
    bayeux::test::context context;

    const std::vector< bayeux::test::response_t > response = bayeux::test::bayeux_session(
        server::test::read_plan()
            << bayeux::test::msg(
                "{ 'channel' : '/meta/handshake',"
                "  'version' : '1.0.0',"
                "  'supportedConnectionTypes' : ['long-polling', 'callback-polling'] }" )
            << bayeux::test::msg(
                "{ 'channel' : '/meta/subscribe',"
                "  'subscription' : '/foo/bar' }" )
            << server::test::disconnect_read(),
        context );

    BOOST_REQUIRE_EQUAL( 2u, response.size() );

    BOOST_CHECK_EQUAL( response[ 0 ].second, json::parse_single_quoted(
        "["
        "   {"
        "       'channel'       : '/meta/handshake',"
        "       'version'       : '1.0',"
        "       'clientId'      : '192.168.210.1:9999/0',"
        "       'successful'    : true,"
        "       'supportedConnectionTypes': ['long-polling']"
        "   }"
        "]" ) );

    BOOST_CHECK_EQUAL( response[ 1 ].second, json::parse_single_quoted(
        "["
        "   {"
        "       'channel'       : '/meta/subscribe',"
        "       'successful'    : false,"
        "       'error'         : 'invalid clientId'"
        "   }"
        "]" ) );
}

/**
 * @test subscribe with invalid client-id must fail
 */
BOOST_AUTO_TEST_CASE( subscribe_with_invalid_client_id )
{
    bayeux::test::context context;

    const std::vector< bayeux::test::response_t > response = bayeux::test::bayeux_session(
        server::test::read_plan()
            << bayeux::test::msg(
                "{ 'channel' : '/meta/handshake',"
                "  'version' : '1.0.0',"
                "  'supportedConnectionTypes' : ['long-polling', 'callback-polling'] }" )
            << bayeux::test::msg(
                "{ 'channel' : '/meta/subscribe',"
                "  'subscription' : '/foo/bar',"
                "  'clientId'     : 'xxxxx' }" )
            << server::test::disconnect_read(),
        context );

    BOOST_REQUIRE_EQUAL( 2u, response.size() );

    BOOST_CHECK_EQUAL( response[ 0 ].second, json::parse_single_quoted(
        "["
        "   {"
        "       'channel'       : '/meta/handshake',"
        "       'version'       : '1.0',"
        "       'clientId'      : '192.168.210.1:9999/0',"
        "       'successful'    : true,"
        "       'supportedConnectionTypes': ['long-polling']"
        "   }"
        "]" ) );

    BOOST_CHECK_EQUAL( response[ 1 ].second, json::parse_single_quoted(
        "["
        "   {"
        "       'channel'       : '/meta/subscribe',"
        "       'successful'    : false,"
        "       'error'         : 'invalid clientId',"
        "       'clientId'      : 'xxxxx'"
        "   }"
        "]" ) );
}

/**
 * @test check that a subscribed client get updates an that an unsubscribed client does not.
 */
BOOST_AUTO_TEST_CASE( unsubscribe_after_subscription )
{
    bayeux::test::context context( pubsub::configurator().authorization_not_required() );

    context.adapter.answer_validation_request( bayeux::node_name_from_channel( "/foo/bar" ), true );
    context.adapter.answer_initialization_request( bayeux::node_name_from_channel( "/foo/bar" ), json::number( 41 ) );

    const json::array response = bayeux::test::bayeux_messages( bayeux::test::bayeux_session(
        server::test::read_plan()
            << bayeux::test::msg(
                "{ 'channel' : '/meta/handshake',"
                "  'version' : '1.0.0',"
                "  'supportedConnectionTypes' : ['long-polling', 'callback-polling'] }" )
            << bayeux::test::msg(
                "{ 'channel' : '/meta/subscribe',"
                "  'clientId' : '192.168.210.1:9999/0',"
                "  'subscription' : '/foo/bar' }" )
            << bayeux::test::msg(
                "{ 'channel' : '/meta/connect',"
                "  'clientId' : '192.168.210.1:9999/0',"
                "  'connectionType' : 'long-polling' }" )
            << update_node( context, "/foo/bar", json::number( 42 ) )
            << bayeux::test::msg(
                "{ 'channel' : '/meta/unsubscribe',"
                "  'clientId' : '192.168.210.1:9999/0',"
                "  'subscription' : '/foo/bar' }" )
            << bayeux::test::msg(
                "{ 'channel' : '/meta/connect',"
                "  'clientId' : '192.168.210.1:9999/0',"
                "  'connectionType' : 'long-polling' }" )
            << update_node( context, "/foo/bar", json::number( 43 ) )
            << server::test::disconnect_read(),
        server::test::write_plan(),
        context ) );

    BOOST_CHECK_EQUAL( response,
        json::parse_single_quoted(
            "["
            "   {"
            "       'channel'       : '/meta/handshake',"
            "       'version'       : '1.0',"
            "       'clientId'      : '192.168.210.1:9999/0',"
            "       'successful'    : true,"
            "       'supportedConnectionTypes' : ['long-polling']"
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
            "   },"
            "   {"
            "       'data'          : 42,"
            "       'channel'       : '/foo/bar'"
            "   },"
            "   {"
            "       'channel'       : '/meta/unsubscribe',"
            "       'subscription'   : '/foo/bar',"
            "       'clientId'      : '192.168.210.1:9999/0',"
            "       'successful'    : true"
            "   },"
            "   {"
            "       'channel'       : '/meta/connect',"
            "       'clientId'      : '192.168.210.1:9999/0',"
            "       'successful'    : true"
            "   }"
            "]" )
    );
}

/**
 * @test an unsubscribe from a not subscribed node should be flagged as an error
 */
BOOST_AUTO_TEST_CASE( unsubscribe_without_beeing_subscribed )
{
    bayeux::test::context context;

    const json::array response = bayeux::test::bayeux_messages( bayeux::test::bayeux_session(
        server::test::read_plan()
            << bayeux::test::msg(
                "{ 'channel' : '/meta/handshake',"
                "  'version' : '1.0.0',"
                "  'supportedConnectionTypes' : ['long-polling', 'callback-polling'] }" )
            << bayeux::test::msg(
                "{ 'channel' : '/meta/unsubscribe',"
                "  'clientId' : '192.168.210.1:9999/0',"
                "  'subscription' : '/foo/bar' }" )
            << server::test::disconnect_read(),
        server::test::write_plan(),
        context ) );

    BOOST_CHECK_EQUAL( response,
        json::parse_single_quoted(
            "["
            "   {"
            "       'channel'       : '/meta/handshake',"
            "       'version'       : '1.0',"
            "       'clientId'      : '192.168.210.1:9999/0',"
            "       'successful'    : true,"
            "       'supportedConnectionTypes' : ['long-polling']"
            "   },"

            "   {"
            "       'channel'       : '/meta/unsubscribe',"
            "       'subscription'   : '/foo/bar',"
            "       'clientId'      : '192.168.210.1:9999/0',"
            "       'successful'    : false,"
            "       'error'         : 'not subscribed'"
            "   }"
            "]" )
    );
}

BOOST_AUTO_TEST_CASE( unsubscribe_without_beeing_subscribed_with_id )
{
    bayeux::test::context context;

    const json::array response = bayeux::test::bayeux_messages( bayeux::test::bayeux_session(
        server::test::read_plan()
            << bayeux::test::msg(
                "{ 'channel' : '/meta/handshake',"
                "  'version' : '1.0.0',"
                "  'supportedConnectionTypes' : ['long-polling', 'callback-polling'] }" )
            << bayeux::test::msg(
                "{  "
                "   'channel'       : '/meta/unsubscribe',"
                "   'clientId'      : '192.168.210.1:9999/0',"
                "   'subscription'  : '/foo/bar',"
                "   'id'            : { 'a': 15 }"
                "}" )
            << server::test::disconnect_read(),
        server::test::write_plan(),
        context ) );

    BOOST_CHECK_EQUAL( response,
        json::parse_single_quoted(
            "["
            "   {"
            "       'channel'       : '/meta/handshake',"
            "       'version'       : '1.0',"
            "       'clientId'      : '192.168.210.1:9999/0',"
            "       'successful'    : true,"
            "       'supportedConnectionTypes' : ['long-polling']"
            "   },"

            "   {"
            "       'channel'       : '/meta/unsubscribe',"
            "       'subscription'   : '/foo/bar',"
            "       'clientId'      : '192.168.210.1:9999/0',"
            "       'successful'    : false,"
            "       'error'         : 'not subscribed',"
            "       'id'            : { 'a': 15 }"
            "   }"
            "]" )
    );
}

BOOST_AUTO_TEST_CASE( unsubscribe_without_subject )
{
    bayeux::test::context context;

    const json::array response = bayeux::test::bayeux_messages( bayeux::test::bayeux_session(
        server::test::read_plan()
            << bayeux::test::msg(
                "{ 'channel' : '/meta/handshake',"
                "  'version' : '1.0.0',"
                "  'supportedConnectionTypes' : ['long-polling', 'callback-polling'] }" )
            << bayeux::test::msg(
                "{ 'channel' : '/meta/unsubscribe',"
                "  'clientId' : '192.168.210.1:9999/0' }" )
            << server::test::disconnect_read(),
        server::test::write_plan(),
        context ) );

    BOOST_CHECK_EQUAL( response,
        json::parse_single_quoted(
            "["
            "   {"
            "       'channel'       : '/meta/handshake',"
            "       'version'       : '1.0',"
            "       'clientId'      : '192.168.210.1:9999/0',"
            "       'successful'    : true,"
            "       'supportedConnectionTypes' : ['long-polling']"
            "   },"

            "   {"
            "       'channel'       : '/meta/unsubscribe',"
            "       'clientId'      : '192.168.210.1:9999/0',"
            "       'successful'    : false,"
            "       'error'         : 'not subscribed',"
            "       'subscription'  : ''"
            "   }"
            "]" )
    );
}

/**
 * @test unsubscribe without client-id must fail
 */
BOOST_AUTO_TEST_CASE( unsubscribe_without_client_id )
{
    bayeux::test::context context;

    const json::array response = bayeux::test::bayeux_messages( bayeux::test::bayeux_session(
        server::test::read_plan()
            << bayeux::test::msg(
                "{ 'channel' : '/meta/handshake',"
                "  'version' : '1.0.0',"
                "  'supportedConnectionTypes' : ['long-polling', 'callback-polling'] }" )
            << bayeux::test::msg(
                "{ 'channel' : '/meta/unsubscribe',"
                "  'subscription' : '/foo/bar'  }" )
            << server::test::disconnect_read(),
        server::test::write_plan(),
        context ) );

    BOOST_CHECK_EQUAL( response,
        json::parse_single_quoted(
            "["
            "   {"
            "       'channel'       : '/meta/handshake',"
            "       'version'       : '1.0',"
            "       'clientId'      : '192.168.210.1:9999/0',"
            "       'successful'    : true,"
            "       'supportedConnectionTypes' : ['long-polling']"
            "   },"

            "   {"
            "       'channel'       : '/meta/unsubscribe',"
            "       'successful'    : false,"
            "       'error'         : 'invalid clientId'"
            "   }"
            "]" )
    );
}

/**
 * @test subscribe with invalid client-id must fail
 */
BOOST_AUTO_TEST_CASE( unsubscribe_with_invalid_client_id )
{
    bayeux::test::context context;

    const json::array response = bayeux::test::bayeux_messages( bayeux::test::bayeux_session(
        server::test::read_plan()
            << bayeux::test::msg(
                "{ 'channel' : '/meta/handshake',"
                "  'version' : '1.0.0',"
                "  'supportedConnectionTypes' : ['long-polling', 'callback-polling'] }" )
            << bayeux::test::msg(
                "{ 'channel' : '/meta/unsubscribe',"
                "  'clientId'      : 'xxxxx',"
                "  'subscription' : '/foo/bar'  }" )
            << server::test::disconnect_read(),
        server::test::write_plan(),
        context ) );

    BOOST_CHECK_EQUAL( response,
        json::parse_single_quoted(
            "["
            "   {"
            "       'channel'       : '/meta/handshake',"
            "       'version'       : '1.0',"
            "       'clientId'      : '192.168.210.1:9999/0',"
            "       'successful'    : true,"
            "       'supportedConnectionTypes' : ['long-polling']"
            "   },"

            "   {"
            "       'channel'       : '/meta/unsubscribe',"
            "       'clientId'      : 'xxxxx',"
            "       'successful'    : false,"
            "       'error'         : 'invalid clientId'"
            "   }"
            "]" )
    );
}

/**
 * @test test that a bayeux-connect blocks if nothing is there to be send
 *
 * The test is based on the current implementation, where a subscription will not directly respond.
 * The first connect will collect the subscribe respond from the first message, the second connect will
 * then block until the update is received.
 */
BOOST_AUTO_TEST_CASE( bayeux_connect_blocks_until_an_event_happens )
{
    bayeux::test::context context( pubsub::configurator().authorization_not_required() );

    context.adapter.answer_validation_request( bayeux::node_name_from_channel( "/foo/bar" ), true );
    context.adapter.answer_initialization_request( bayeux::node_name_from_channel( "/foo/bar" ), json::null() );

    json::array response = bayeux::test::bayeux_messages( bayeux::test::bayeux_session(
        server::test::read_plan()
            << bayeux::test::msg(
                "[{ "
                "   'channel' : '/meta/handshake',"
                "   'version' : '1.0.0',"
                "   'supportedConnectionTypes' : ['long-polling', 'callback-polling']"
                "},{ "
                "   'channel' : '/meta/subscribe',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'subscription' : '/foo/bar' "
                "}]" )
            << bayeux::test::msg(
                "{ "
                "   'channel' : '/meta/connect',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'connectionType' : 'long-polling'"
                "}" )
            << bayeux::test::msg(
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
    bayeux::test::context context( pubsub::configurator().authorization_not_required() );

    context.adapter.answer_validation_request( bayeux::node_name_from_channel( "/foo/bar" ), true );
    context.adapter.answer_initialization_request( bayeux::node_name_from_channel( "/foo/bar" ), json::null() );

    bayeux::test::bayeux_session(
        server::test::read_plan()
            << bayeux::test::msg(
                "[{ "
                "   'channel' : '/meta/handshake',"
                "   'version' : '1.0.0',"
                "   'supportedConnectionTypes' : ['long-polling', 'callback-polling']"
                "},{ "
                "   'channel' : '/meta/subscribe',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'subscription' : '/foo/bar' "
                "}]" )
            << bayeux::test::msg(
                "{ "
                "   'channel' : '/meta/connect',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'connectionType' : 'long-polling'"
                "}" )
             << server::test::disconnect_read(),
        server::test::write_plan(),
        context,
        boost::posix_time::seconds( 1 ) );

    bayeux::test::socket_t    socket(
        context.queue,
        server::test::read_plan()
            << bayeux::test::msg(
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

    typedef server::connection< bayeux::test::trait_t >                           connection_t;
    boost::shared_ptr< connection_t > connection( new connection_t( socket, context.trait ) );
    connection->start();

    tools::run( context.queue );

    // now the session should still be available
    bayeux::session* session =
        context.trait.connector_.find_session( json::string( "192.168.210.1:9999/0" ) );

    BOOST_REQUIRE( session );
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
    const std::vector< bayeux::test::response_t > response = bayeux::test::bayeux_session(
        server::test::read_plan()
            << bayeux::test::msg("[{]") // a somehow broken message
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

    bool occurences_in_range( const std::vector< bayeux::test::response_t >& response, const char* field,
        const char* value, unsigned min, unsigned max )
    {
        const json::string field_name( field );
        const json::value field_value = json::parse_single_quoted( value );

        bool result = true;
        for ( std::vector< bayeux::test::response_t >::const_iterator i = response.begin();
            i != response.end() && result; ++i )
        {
            const unsigned count = count_fields( i->second, field_name, field_value );
            result = count >= min && count <= max;
        }

        return result;
    }

    bool contains_at_least_once( const std::vector< bayeux::test::response_t >& response, const char* field,
        const char* value )
    {
        return occurences_in_range( response, field, value, 1u, 100000000u );
    }

    bool contains_not( const std::vector< bayeux::test::response_t >& response, const char* field, const char* value )
    {
        return occurences_in_range( response, field, value, 0, 0 );
    }
}

/**
 * @test a http proxy could use one http connection to connect more than one client to the bayeux server
 */
BOOST_AUTO_TEST_CASE( more_than_one_session_in_a_single_connection )
{
    bayeux::test::context context( pubsub::configurator().authorization_not_required() );

    context.adapter.answer_validation_request( bayeux::node_name_from_channel( "/foo/bar" ), true );
    context.adapter.answer_initialization_request( bayeux::node_name_from_channel( "/foo/bar" ), json::null() );

    const std::vector< bayeux::test::response_t > response = bayeux::test::bayeux_session(
        server::test::read_plan()
            << bayeux::test::msg(
                "{"
                "   'channel' : '/meta/handshake',"
                "   'version' : '1.0.0',"
                "   'supportedConnectionTypes' : ['long-polling', 'callback-polling'],"
                "   'id'      : 'id_first_handshake'"
                "}" )
            << bayeux::test::msg(
                "[{"
                "   'channel' : '/meta/subscribe',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'subscription' : '/foo/bar' "
                "},{ "
                "   'channel' : '/meta/connect',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'connectionType' : 'long-polling' "
                "}]" )
            << bayeux::test::msg(
                "[{ "
                "   'channel' : '/meta/handshake',"
                "   'version' : '1.0.0',"
                "   'supportedConnectionTypes' : ['long-polling', 'callback-polling'],"
                "   'id'      : 'id_second_handshake'"
                "}]")
            << bayeux::test::msg(
                "[{ "
                "   'channel'      : '/meta/subscribe',"
                "   'clientId'     : '192.168.210.1:9999/1',"
                "   'subscription' : '/foo/bar' "
                "},{ "
                "   'channel'      : '/meta/connect',"
                "   'clientId'     : '192.168.210.1:9999/1',"
                "   'connectionType' : 'long-polling' "
                "}]" )
            << bayeux::test::msg(
                "[{ "
                "   'channel' : '/meta/connect',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'connectionType' : 'long-polling'"
                "}]" )
            << bayeux::test::msg(
                "[{ "
                "   'channel'  : '/meta/connect',"
                "   'clientId' : '192.168.210.1:9999/1',"
                "   'connectionType' : 'long-polling'"
                "}]" )
             << update_node( context, "/foo/bar", json::number( 42 ) )
             << server::test::disconnect_read(),
        context );

    BOOST_REQUIRE_EQUAL( 6u, response.size() );
    std::vector< bayeux::test::response_t > response_first_client;
    std::vector< bayeux::test::response_t > response_second_client;
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
 * @brief hurry a waiting connection, if a normal http request is pipelined
 *
 * The second http request should block because it contains a connect.
 */
BOOST_AUTO_TEST_CASE( hurry_bayeux_connection_if_request_is_pipelined )
{
    bayeux::test::context context( pubsub::configurator().authorization_not_required() );

    const std::vector< bayeux::test::response_t > response = bayeux::test::bayeux_session(
        server::test::read_plan()
            << bayeux::test::msg(
                "{"
                "   'channel' : '/meta/handshake',"
                "   'version' : '1.0.0',"
                "   'supportedConnectionTypes' : ['long-polling', 'callback-polling'],"
                "   'id'      : 'id_first_handshake'"
                "}" )
            << bayeux::test::msg(
                "[{"
                "   'channel' : '/meta/subscribe',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'subscription' : '/foo/bar' "
                "},{ "
                "   'channel' : '/meta/connect',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'connectionType' : 'long-polling' "
                "}]" )
            << bayeux::test::msg(
                "[{"
                "   'channel' : '/meta/subscribe',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'subscription' : '/foo/chu' "
                "}]" )
            << server::test::disconnect_read(),
            context );

    BOOST_REQUIRE_EQUAL( response.size(), 3u );
    BOOST_CHECK_EQUAL(
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
    bayeux::test::context context( pubsub::configurator().authorization_not_required() );
    const boost::posix_time::ptime   start_time = server::test::current_time();

    context.adapter.answer_validation_request( bayeux::node_name_from_channel( "/foo/bar" ), true );
    context.adapter.answer_initialization_request( bayeux::node_name_from_channel( "/foo/bar" ), json::null() );

    const std::vector< bayeux::test::response_t > response = bayeux::test::bayeux_session(
        server::test::read_plan()
            << bayeux::test::msg(
                "{"
                "   'channel' : '/meta/handshake',"
                "   'version' : '1.0.0',"
                "   'supportedConnectionTypes' : ['long-polling', 'callback-polling']"
                "}" )
            << bayeux::test::msg(
                "[{ "
                "   'channel'           : '/meta/connect',"
                "   'clientId'          : '192.168.210.1:9999/0',"
                "   'connectionType'    : 'long-polling' "
                "},"
                "{"
                "   'channel'           : '/meta/subscribe',"
                "   'clientId'          : '192.168.210.1:9999/0',"
                "   'subscription'      : '/foo/bar' "
                "}]" )
           << server::test::disconnect_read(),
        server::test::write_plan(),
        context );

    BOOST_REQUIRE_EQUAL( 2u, response.size() );

    BOOST_CHECK_EQUAL( response[ 1u ].first->code(), http::http_ok );
    BOOST_CHECK_EQUAL(
        response[ 1 ].second,
        json::parse_single_quoted(
            "["
            "   {"
            "       'channel'       : '/meta/connect',"
            "       'clientId'      : '192.168.210.1:9999/0',"
            "       'successful'    : true"
            "   }"
            "]" ) );

    BOOST_CHECK_EQUAL( response[ 1u ].received, start_time );
}

/**
 * @test a connect should not block for ever, but instead for the configured poll timeout
 */
BOOST_AUTO_TEST_CASE( long_poll_time_out_test )
{
    const boost::posix_time::seconds timeout( 100 );

    bayeux::test::context context(
        pubsub::configurator().authorization_not_required(),
        bayeux::configuration().long_polling_timeout( timeout ) );

    const boost::posix_time::ptime   start_time = server::test::current_time();

    const std::vector< bayeux::test::response_t > response = bayeux::test::bayeux_session(
        server::test::read_plan()
            << bayeux::test::msg(
                "{"
                "   'channel' : '/meta/handshake',"
                "   'version' : '1.0.0',"
                "   'supportedConnectionTypes' : ['long-polling', 'callback-polling'],"
                "   'id'      : 'id_first_handshake'"
                "}" )
            << bayeux::test::msg(
                "[{ "
                "   'channel' : '/meta/connect',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'connectionType' : 'long-polling' "
                "}]" )
           << server::test::disconnect_read(),
       server::test::write_plan(),
       context,
       boost::posix_time::minutes( 5 ) );

    BOOST_REQUIRE_EQUAL( 2u, response.size() );

    BOOST_CHECK_EQUAL( response[ 1u ].first->code(), http::http_ok );
    BOOST_CHECK_EQUAL(
        response[ 1 ].second,
        json::parse_single_quoted(
            "["
            "   {"
            "       'channel'       : '/meta/connect',"
            "       'clientId'      : '192.168.210.1:9999/0',"
            "       'successful'    : true"
            "   }"
            "]" ) );
    BOOST_CHECK_EQUAL( response[ 1u ].received - start_time, timeout );
}

/**
 * @test a bayeux disconnect message is somehow pointless, but should be implemented
 */
BOOST_AUTO_TEST_CASE( disconnect_test )
{
    const std::vector< bayeux::test::response_t > response = bayeux::test::bayeux_session(
        server::test::read_plan()
            << bayeux::test::msg(
                "{"
                "   'channel' : '/meta/handshake',"
                "   'version' : '1.0.0',"
                "   'supportedConnectionTypes' : ['long-polling', 'callback-polling']"
                "}" )
            << bayeux::test::msg(
                "{ "
                "   'channel' : '/meta/connect',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'connectionType' : 'long-polling' "
                "}" )
            << bayeux::test::msg(
                "{ "
                "   'channel' : '/meta/disconnect',"
                "   'clientId' : '192.168.210.1:9999/0'"
                "}" )
           << server::test::disconnect_read() );

    BOOST_REQUIRE_EQUAL( 3u, response.size() );

    BOOST_CHECK_EQUAL( response[ 2u ].first->code(), http::http_ok );
    BOOST_CHECK_EQUAL(
        response[ 2u ].second,
        json::parse_single_quoted(
            "["
            "   {"
            "       'channel'       : '/meta/disconnect',"
            "       'clientId'      : '192.168.210.1:9999/0',"
            "       'successful'    : true"
            "   }"
            "]" ) );
}

/**
 * @test a disconnect with id field should contain the id field in the response
 */
BOOST_AUTO_TEST_CASE( disconnect_with_id_test )
{
    const std::vector< bayeux::test::response_t > response = bayeux::test::bayeux_session(
        server::test::read_plan()
            << bayeux::test::msg(
                "{"
                "   'channel' : '/meta/handshake',"
                "   'supportedConnectionTypes' : ['long-polling'],"
                "   'version' : '1.0.0'"
                "}" )
            << bayeux::test::msg(
                "{ "
                "   'channel' : '/meta/connect',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'connectionType' : 'long-polling' "
                "}" )
            << bayeux::test::msg(
                "{ "
                "   'channel' : '/meta/disconnect',"
                "   'id'      : { 'sub' : 42 },"
                "   'clientId' : '192.168.210.1:9999/0'"
                "}" )
           << server::test::disconnect_read() );

    BOOST_REQUIRE_EQUAL( 3u, response.size() );

    BOOST_CHECK_EQUAL( response[ 2u ].first->code(), http::http_ok );
    BOOST_CHECK_EQUAL(
        response[ 2u ].second,
        json::parse_single_quoted(
            "["
            "   {"
            "       'channel'       : '/meta/disconnect',"
            "       'clientId'      : '192.168.210.1:9999/0',"
            "       'id'            : { 'sub' : 42 },"
            "       'successful'    : true"
            "   }"
            "]" ) );
}

/**
 * @test a disconnect without client id should be flagged as error
 */
BOOST_AUTO_TEST_CASE( disconnect_without_client_id )
{
    const std::vector< bayeux::test::response_t > response = bayeux::test::bayeux_session(
        server::test::read_plan()
            << bayeux::test::msg(
                "{ "
                "   'channel' : '/meta/disconnect',"
                "   'clientId' : '192.168.210.1:9999/0'"
                "}" )
           << server::test::disconnect_read() );

    BOOST_REQUIRE_EQUAL( 1u, response.size() );

    BOOST_CHECK_EQUAL( response[ 0u ].first->code(), http::http_ok );
    BOOST_CHECK_EQUAL(
        response[ 0u ].second,
        json::parse_single_quoted(
            "["
            "   {"
            "       'channel'       : '/meta/disconnect',"
            "       'successful'    : false,"
            "       'error'         : 'invalid clientId',"
            "       'clientId'      : '192.168.210.1:9999/0'"
            "   }"
            "]" ) );
}

/**
 * @test a disconnect within a bayeux message array should not result in a connected session
 */
BOOST_AUTO_TEST_CASE( connect_packed_with_disconnect )
{
    const std::vector< bayeux::test::response_t > response = bayeux::test::bayeux_session(
        server::test::read_plan()
            << bayeux::test::msg(
                "{"
                "   'channel' : '/meta/handshake',"
                "   'supportedConnectionTypes' : ['long-polling'],"
                "   'version' : '1.0.0'"
                "}" )
            << bayeux::test::msg(
                "[{ "
                "   'channel' : '/meta/connect',"
                "   'clientId' : '192.168.210.1:9999/0',"
                "   'connectionType' : 'long-polling' "
                "},"
                "{ "
                "   'channel' : '/meta/disconnect',"
                "   'clientId' : '192.168.210.1:9999/0'"
                "}]" )
           << server::test::disconnect_read() );

    BOOST_REQUIRE_EQUAL( 2u, response.size() );

    BOOST_CHECK_EQUAL( response[ 1u ].first->code(), http::http_ok );
    BOOST_CHECK_EQUAL(
        response[ 1u ].second,
        json::parse_single_quoted(
            "["
            "   {"
            "       'channel'       : '/meta/connect',"
            "       'clientId'      : '192.168.210.1:9999/0',"
            "       'successful'    : true"
            "   },"
            "   {"
            "       'channel'       : '/meta/disconnect',"
            "       'clientId'      : '192.168.210.1:9999/0',"
            "       'successful'    : true"
            "   }"
            "]" ) );
}

