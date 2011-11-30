// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include <boost/test/unit_test.hpp>

#include "bayeux/configuration.h"
#include "bayeux/node_channel.h"
#include "bayeux/session.h"
#include "pubsub/configuration.h"
#include "pubsub/root.h"
#include "pubsub/test_helper.h"
#include "tools/asstring.h"
#include "tools/io_service.h"

#include <vector>

// delete me if you see me
#include <iostream>

namespace {

    boost::shared_ptr< const bayeux::configuration > config()
    {
        return boost::shared_ptr< const bayeux::configuration >( new bayeux::configuration );
    }

    boost::shared_ptr< const bayeux::configuration > config( const bayeux::configuration& c )
    {
        return boost::shared_ptr< const bayeux::configuration >( new bayeux::configuration( c ) );
    }

    struct test_root
    {
        boost::asio::io_service io_queue_;
        pubsub::test::adapter   adapter_;
        pubsub::root            root_;

        test_root()
            : io_queue_()
            , adapter_()
            , root_( io_queue_, adapter_, pubsub::configuration() )
        {
        }
    };

    const pubsub::node_name node_1( bayeux::node_name_from_channel( json::string("/a/b") ) );
    const pubsub::node_name node_2( bayeux::node_name_from_channel( json::string("/foo/bar/chu") ) );
    const pubsub::node_version v1;
    const pubsub::node_version v2 = v1 + 1;
    const pubsub::node_version v3 = v1 + 2;
    const json::value data1 = json::parse_single_quoted("{ 'data':1 }");
    const json::value data2 = json::parse_single_quoted("{ 'data':2 }");
    const json::value data2_with_id = json::parse_single_quoted("{ 'data':2, 'id':'foo' }");
    const json::value data3 = json::parse_single_quoted("{ 'data':3 }");
}

BOOST_AUTO_TEST_CASE( constructor_stores_argument )
{
    test_root       root;
    bayeux::session first_session( "SessionId4711", root.root_, config() );
    BOOST_CHECK_EQUAL( json::string( "SessionId4711" ), first_session.session_id() );
}

BOOST_AUTO_TEST_CASE( check_for_single_node_update )
{
    test_root       root;
	bayeux::session session( "sss", root.root_, config() );
	BOOST_CHECK_EQUAL( json::array(), session.events() );

	static_cast< pubsub::subscriber& >( session ).on_update( node_1, pubsub::node( v1, data1 ) );
	const json::array first_update = session.events();

	BOOST_REQUIRE_EQUAL( 1u, first_update.length() );
	BOOST_CHECK_EQUAL(
		first_update.at( 0 ),
		json::parse_single_quoted(
				"{"
				"	'channel' : '/a/b',"
				"	'data' : 1 "
				"}") );

	static_cast< pubsub::subscriber& >( session ).on_update( node_1, pubsub::node( v1, data2_with_id ) );
	const json::array second_update = session.events();

	BOOST_REQUIRE_EQUAL( 1u, second_update.length() );
	BOOST_CHECK_EQUAL(
		second_update.at( 0 ),
		json::parse_single_quoted(
				"{"
				"	'channel' : '/a/b',"
				"	'data' : 2, "
				"   'id'   : 'foo' "
				"}") );

	const json::array third_update = session.events();
	BOOST_CHECK_EQUAL( 0, third_update.length() );
}

BOOST_AUTO_TEST_CASE( check_for_multiple_updates_on_a_single_node )
{
    test_root       root;
	bayeux::session session( "sss", root.root_, config() );
	BOOST_CHECK_EQUAL( json::array(), session.events() );

	static_cast< pubsub::subscriber& >( session ).on_update( node_1, pubsub::node( v1, data1 ) );
	static_cast< pubsub::subscriber& >( session ).on_update( node_1, pubsub::node( v1, data2_with_id ) );
	const json::array first_update = session.events();

	BOOST_REQUIRE_EQUAL( 2u, first_update.length() );
	BOOST_CHECK_EQUAL(
		first_update,
		json::parse_single_quoted(
				"[{"
				"	'channel' : '/a/b',"
				"	'data' : 1 "
				"},{"
				"	'channel' : '/a/b',"
				"	'data' : 2, "
				"   'id'   : 'foo' "
				"}]") );

	const json::array second_update = session.events();

	BOOST_CHECK_EQUAL( 0u, second_update.length() );
}

/**
 * @test make sure, that multiple pushes with the same content are published without optimization
 */
BOOST_AUTO_TEST_CASE( check_for_multiple_identical_pushes_on_a_single_node )
{
    test_root       root;
	bayeux::session session( "sss", root.root_, config() );
	BOOST_CHECK_EQUAL( json::array(), session.events() );

	static_cast< pubsub::subscriber& >( session ).on_update( node_1, pubsub::node( v1, data1 ) );
	static_cast< pubsub::subscriber& >( session ).on_update( node_1, pubsub::node( v2, data1 ) );
	static_cast< pubsub::subscriber& >( session ).on_update( node_1, pubsub::node( v3, data1 ) );
	const json::array first_update = session.events();

	BOOST_REQUIRE_EQUAL( 3u, first_update.length() );
	BOOST_CHECK_EQUAL(
		first_update,
		json::parse_single_quoted(
				"[{"
				"	'channel' : '/a/b',"
				"	'data' : 1 "
				"},{"
				"	'channel' : '/a/b',"
				"	'data' : 1 "
				"},{"
				"	'channel' : '/a/b',"
				"	'data' : 1 "
				"}]") );

	const json::array second_update = session.events();

	BOOST_CHECK_EQUAL( 0u, second_update.length() );
}

static void subscribe_session(
    test_root& context, const boost::shared_ptr< pubsub::subscriber >& session, const pubsub::node_name& node )
{
    context.adapter_.answer_validation_request( node, true );
    context.adapter_.answer_authorization_request( session, node, true );
    context.adapter_.answer_initialization_request( node, json::null() );

    static_cast< bayeux::session& >( *session ).subscribe( node, 0 );
    tools::run( context.io_queue_ );

    const json::array response = static_cast< bayeux::session& >( *session ).events();
    BOOST_REQUIRE_EQUAL( 1u, response.length() );

    const json::object acknowlage = response.at( 0 ).upcast< json::object >();
    BOOST_REQUIRE_EQUAL( acknowlage.at( json::string("successful") ), json::true_val() );
}

/**
 * @test the number of messages stored, have to be limited
 */
BOOST_AUTO_TEST_CASE( check_that_update_history_is_limited )
{
    test_root       root;
    const boost::shared_ptr< bayeux::session > session(
        new bayeux::session( "sss", root.root_, config( bayeux::configuration().max_messages_per_client( 2u ) ) ) );

	subscribe_session( root, session, node_1 );

	root.root_.update_node( node_1, data1 );
    root.root_.update_node( node_1, data2 );
    root.root_.update_node( node_1, data3 );

    const json::array first_update = session->events();

	BOOST_CHECK_EQUAL( 2u, first_update.length() );
	BOOST_CHECK_EQUAL(
		first_update,
		json::parse_single_quoted(
				"[{"
				"	'channel' : '/a/b',"
				"	'data' : 2 "
				"},{"
				"	'channel' : '/a/b',"
				"	'data' : 3 "
				"}]") );

	const json::array second_update = session->events();

	BOOST_CHECK_EQUAL( 0u, second_update.length() );
}

/**
 * @test the total messages size stored, is limited
 */
BOOST_AUTO_TEST_CASE( total_message_size_limited )
{
    const unsigned message_limit = 10u * 1024u;

    test_root       root;
    boost::shared_ptr< bayeux::session > session( new bayeux::session( "sss", root.root_, config(
        bayeux::configuration()
            .max_messages_per_client( message_limit ) // make sure, that this is not limiting
            .max_messages_size_per_client( message_limit ) ) ) );

    subscribe_session( root, session, node_1 );

    for ( unsigned count = 0; count != message_limit; ++count )
    {
        json::object message;
        message.add( json::string( "data" ), json::number( static_cast< int >( count ) ) );
        root.root_.update_node( node_1, message );
    }

    const json::array update = session->events();
    BOOST_CHECK_LE( update.size(), message_limit );
    BOOST_CHECK_GT( update.size(), message_limit * 9 / 10 );

    const json::number first_element = update.at( 0 ).upcast< json::object >().at( json::string( "data" ) )
        .upcast< json::number >();
    const json::number last_element = update.at( update.length() -1 ).upcast< json::object >()
        .at( json::string( "data" ) ).upcast< json::number >();

    BOOST_CHECK_LT( first_element.to_int(), last_element.to_int() );
}

namespace
{
    class test_interface : public bayeux::response_interface
    {
    public:
        test_interface()
            : second_connections_detected_( 0 )
        {
        }

        int number_of_second_connection_detected() const
        {
            return second_connections_detected_;
        }

        json::array new_message() const
        {
            if ( messages_.size() != 1u )
                throw std::runtime_error( "no single message reported: " + tools::as_string( messages_.size() ) );

            return messages_.front();
        }

        const std::vector< json::array >& messages() const
        {
            return messages_;
        }

    private:
        void second_connection_detected()
        {
            ++second_connections_detected_;
        }

        void new_messages( const json::array& message )
        {
            messages_.push_back( message );
        }

        int                         second_connections_detected_;
        std::vector< json::array >  messages_;
    };
}

/**
 * @test notify a connected and asynchronous http response, when updates come in
 */
BOOST_AUTO_TEST_CASE( response_notified_by_session_when_messages_come_in )
{
    test_root       root;
    bayeux::session session( "sss", root.root_, config() );
    boost::shared_ptr< test_interface > response( new test_interface );

    BOOST_CHECK_EQUAL( json::array(), session.wait_for_events( response ) );
    BOOST_CHECK( response->messages().empty() );
    BOOST_CHECK( !response.unique() );

    static_cast< pubsub::subscriber& >( session ).on_update( node_1, pubsub::node( v1, data1 ) );
    BOOST_CHECK_EQUAL(
        response->new_message(),
        json::parse_single_quoted(
                "[{"
                "   'channel' : '/a/b',"
                "   'data' : 1 "
                "}]") );

    BOOST_CHECK( response.unique() );
    BOOST_CHECK( session.events().empty() );

    // no more messages until the response is connected to the
    static_cast< pubsub::subscriber& >( session ).on_update( node_1, pubsub::node( v1, data1 ) );
    BOOST_CHECK_EQUAL( response->messages().size(), 1u );
    BOOST_CHECK( !session.events().empty() );
}

/**
 * @test if there are already messages stored, the wait_for_event() must return with that data and not keep a reference
 *       to the response
 */
BOOST_AUTO_TEST_CASE( respose_not_referenced_if_there_is_alread_data_to_be_send )
{
    test_root       root;
    bayeux::session session( "sss", root.root_, config() );

    boost::shared_ptr< test_interface > response( new test_interface );

    static_cast< pubsub::subscriber& >( session ).on_update( node_1, pubsub::node( v1, data1 ) );
    BOOST_CHECK_EQUAL(
        session.wait_for_events( response ),
        json::parse_single_quoted(
                "[{"
                "   'channel' : '/a/b',"
                "   'data' : 1 "
                "}]") );

    BOOST_CHECK( response->messages().empty() );
    BOOST_CHECK( response.unique() );
}

/**
 * @test if two http connections connect to the very same bayeux session, this situation must be detected and handled
 */
BOOST_AUTO_TEST_CASE( detect_double_connect )
{
    test_root       root;
    bayeux::session session( "sss", root.root_, config() );

    boost::shared_ptr< test_interface > responseA( new test_interface );
    boost::shared_ptr< test_interface > responseB( new test_interface );
    BOOST_CHECK_EQUAL( responseA->number_of_second_connection_detected(), 0 );
    BOOST_CHECK_EQUAL( responseB->number_of_second_connection_detected(), 0 );

    BOOST_CHECK_EQUAL( json::array(), session.wait_for_events( responseA ) );
    BOOST_CHECK_EQUAL( json::array(), session.wait_for_events( responseB ) );

    BOOST_CHECK( responseA.unique() );
    BOOST_CHECK( !responseB.unique() );
    BOOST_CHECK_EQUAL( responseA->number_of_second_connection_detected(), 1u );
    BOOST_CHECK_EQUAL( responseB->number_of_second_connection_detected(), 0 );
}

/**
 * @test authorization failures must be communicated
 */
BOOST_AUTO_TEST_CASE( session_authorization_failed )
{
    test_root       root;
    boost::shared_ptr< bayeux::session > session( new bayeux::session( "sss", root.root_, config() ) );
    root.adapter_.answer_validation_request( node_2, true );
    root.adapter_.answer_authorization_request( session, node_2, false );

    session->subscribe( node_2, 0 );

    tools::run( root.io_queue_ );

    BOOST_CHECK_EQUAL(
        session->events(),
        json::parse_single_quoted(
            "[{"
            "   'channel'  : '/meta/subscribe',"
            "   'clientId' : 'sss',"
            "   'subscription': '/foo/bar/chu',"
            "   'successful' : false,"
            "   'error' : 'authorization failed'"
            "}]") );
}

/**
 * @test asynchronous authorization failures must be communicated
 */
BOOST_AUTO_TEST_CASE( async_session_authorization_failed )
{
    test_root       root;
    boost::shared_ptr< bayeux::session > session( new bayeux::session( "sss", root.root_, config() ) );

    session->subscribe( node_2, 0 );

    root.adapter_.answer_validation_request( node_2, true );
    root.adapter_.answer_authorization_request( session, node_2, false );

    tools::run( root.io_queue_ );

    BOOST_CHECK_EQUAL(
        session->events(),
        json::parse_single_quoted(
            "[{"
            "   'channel'  : '/meta/subscribe',"
            "   'clientId' : 'sss',"
            "   'subscription': '/foo/bar/chu',"
            "   'successful' : false,"
            "   'error' : 'authorization failed'"
            "}]") );
}

/**
 * @test node validation failures must be communicated
 */
BOOST_AUTO_TEST_CASE( session_node_validation_failed )
{
    test_root       root;
    boost::shared_ptr< bayeux::session > session( new bayeux::session( "sss", root.root_, config() ) );
    root.adapter_.answer_validation_request( node_2, false );

    session->subscribe( node_2, 0 );

    tools::run( root.io_queue_ );

    BOOST_CHECK_EQUAL(
        session->events(),
        json::parse_single_quoted(
            "[{"
            "   'channel'  : '/meta/subscribe',"
            "   'clientId' : 'sss',"
            "   'subscription': '/foo/bar/chu',"
            "   'successful' : false,"
            "   'error' : 'invalid subscription'"
            "}]") );
}

/**
 * @test asynchronous node validation failures must be communicated
 */
BOOST_AUTO_TEST_CASE( async_session_node_validation_failed )
{
    test_root       root;
    boost::shared_ptr< bayeux::session > session( new bayeux::session( "sss", root.root_, config() ) );

    session->subscribe( node_2, 0 );
    root.adapter_.answer_validation_request( node_2, false );

    tools::run( root.io_queue_ );

    BOOST_CHECK_EQUAL(
        session->events(),
        json::parse_single_quoted(
            "[{"
            "   'channel'  : '/meta/subscribe',"
            "   'clientId' : 'sss',"
            "   'subscription': '/foo/bar/chu',"
            "   'successful' : false,"
            "   'error' : 'invalid subscription'"
            "}]") );
}

/**
 * @test node initialization failures must be communicated
 */
BOOST_AUTO_TEST_CASE( session_node_initialization_failed )
{
    test_root       root;
    boost::shared_ptr< bayeux::session > session( new bayeux::session( "sss", root.root_, config() ) );
    root.adapter_.answer_validation_request( node_2, true );
    root.adapter_.answer_authorization_request( session, node_2, true );
    root.adapter_.skip_initialization_request( node_2 );

    session->subscribe( node_2, 0 );

    tools::run( root.io_queue_ );

    BOOST_CHECK_EQUAL(
        session->events(),
        json::parse_single_quoted(
            "[{"
            "   'channel'  : '/meta/subscribe',"
            "   'clientId' : 'sss',"
            "   'subscription': '/foo/bar/chu',"
            "   'successful' : false,"
            "   'error' : 'initialization failed'"
            "}]") );
}

/**
 * @test asynchronous node initialization failures must be communicated
 */
BOOST_AUTO_TEST_CASE( asyn_session_node_initialization_failed )
{
    test_root       root;
    boost::shared_ptr< bayeux::session > session( new bayeux::session( "sss", root.root_, config() ) );

    session->subscribe( node_2, 0 );

    root.adapter_.answer_validation_request( node_2, true );
    root.adapter_.answer_authorization_request( session, node_2, true );
    root.adapter_.skip_initialization_request( node_2 );

    tools::run( root.io_queue_ );

    BOOST_CHECK_EQUAL(
        session->events(),
        json::parse_single_quoted(
            "[{"
            "   'channel'  : '/meta/subscribe',"
            "   'clientId' : 'sss',"
            "   'subscription': '/foo/bar/chu',"
            "   'successful' : false,"
            "   'error' : 'initialization failed'"
            "}]") );
}

/**
 * @test synchronous subscription success, with initial data
 */
BOOST_AUTO_TEST_CASE( session_node_subscription_success )
{
    test_root       root;
    boost::shared_ptr< bayeux::session > session( new bayeux::session( "sss", root.root_, config() ) );

    session->subscribe( node_2, 0 );

    root.adapter_.answer_validation_request( node_2, true );
    root.adapter_.answer_authorization_request( session, node_2, true );
    root.adapter_.answer_initialization_request( node_2, json::parse( "{\"data\":42}" ) );

    tools::run( root.io_queue_ );

    boost::shared_ptr< test_interface > response( new test_interface );
    BOOST_CHECK_EQUAL(
        session->wait_for_events( response ),
        json::parse_single_quoted(
            "[{"
            "   'channel'  : '/meta/subscribe',"
            "   'clientId' : 'sss',"
            "   'subscription': '/foo/bar/chu',"
            "   'successful' : true"
            "},"
            "{"
            "   'channel'  : '/foo/bar/chu', "
            "   'data' : 42 "
            "}]") );
}

/**
 * @test asynchronous subscription success
 */
BOOST_AUTO_TEST_CASE( asyn_session_node_subscription_success )
{
    test_root       root;
    boost::shared_ptr< bayeux::session > session( new bayeux::session( "sss", root.root_, config() ) );
    boost::shared_ptr< test_interface > response( new test_interface );

    session->subscribe( node_2, 0 );
    BOOST_CHECK( session->wait_for_events( response ).empty() );

    root.adapter_.answer_validation_request( node_2, true );
    root.adapter_.answer_authorization_request( session, node_2, true );
    root.adapter_.answer_initialization_request( node_2, json::parse( "{\"data\":42}" ) );

    tools::run( root.io_queue_ );

    BOOST_CHECK_EQUAL(
        response->new_message(),
        json::parse_single_quoted(
            "[{"
            "   'channel'  : '/meta/subscribe',"
            "   'clientId' : 'sss',"
            "   'subscription': '/foo/bar/chu',"
            "   'successful' : true"
            "},"
            "{"
            "   'channel'  : '/foo/bar/chu', "
            "   'data' : 42 "
            "}]") );
}

/**
 * @test synchronous subscription success, without initial data
 */
BOOST_AUTO_TEST_CASE( session_node_subscription_success_without_data )
{
    test_root       root;
    boost::shared_ptr< bayeux::session > session( new bayeux::session( "sss", root.root_, config() ) );

    session->subscribe( node_2, 0 );

    root.adapter_.answer_validation_request( node_2, true );
    root.adapter_.answer_authorization_request( session, node_2, true );
    root.adapter_.answer_initialization_request( node_2, json::null() );

    tools::run( root.io_queue_ );

    BOOST_CHECK_EQUAL(
        session->events(),
        json::parse_single_quoted(
            "[{"
            "   'channel'  : '/meta/subscribe',"
            "   'clientId' : 'sss',"
            "   'subscription': '/foo/bar/chu',"
            "   'successful' : true"
            "}]") );
}

/**
 * @test some test with id please
 */
BOOST_AUTO_TEST_CASE( session_id_in_synchronous_failed_subscription_response )
{
}

/**
 * @test mixing data and error messages
 */
BOOST_AUTO_TEST_CASE( session_mixed_subscription_messages_and_data )
{
}

/**
 * @test unsubscribe from a node before the prior subscription was acknowledged
 */
BOOST_AUTO_TEST_CASE( unsubscribe_before_subscription_acknowledged )
{
}
