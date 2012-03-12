// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include <boost/test/unit_test.hpp>

#include "bayeux/bayeux.h"
#include "bayeux/configuration.h"
#include "pubsub/configuration.h"
#include "pubsub/root.h"
#include "pubsub/test_helper.h"
#include "server/session_generator.h"
#include "server/test_timer.h"
#include "tools/asstring.h"
#include "tools/io_service.h"

namespace
{
    class test_session_generator : public server::session_generator
    {
    public:
        test_session_generator()
            : nr_( 0 )
        {
        }

        std::string network_name() const
        {
            return network_name_;
        }
    private:
        virtual std::string operator()( const char* network_connection_name )
        {
            ++nr_;
            network_name_ = network_connection_name;

            return tools::as_string( nr_ );
        }

        int         nr_;
        std::string network_name_;
    };

    typedef bayeux::connector< server::test::timer > connector_t;

    template < class Connector >
    bool session_alive( Connector& con, const char *session_id )
    {
        bayeux::session* const session = con.find_session( json::string( session_id ) );
        const bool result = session != 0;

        if ( session )
            con.idle_session( session );

        return result;
    }

    void advance_time( boost::asio::io_service& queue, unsigned delay_in_seconds )
    {
        server::test::advance_time( boost::posix_time::seconds( delay_in_seconds ) );
        tools::run( queue );
    }
}

/*!
 * @test after a session was created, it must be obtainable
 */
BOOST_AUTO_TEST_CASE( create_session_find_session_test )
{
    boost::asio::io_service queue;
    pubsub::test::adapter   adapter;
    pubsub::root            root( queue, adapter, pubsub::configuration() );

    test_session_generator  generator;
    connector_t             connector( queue, root, generator, bayeux::configuration() );

    BOOST_CHECK( connector.find_session( json::string( "1" ) ) == 0 );

    bayeux::session* session = connector.create_session( "foobar" );
    connector.idle_session( session );

    BOOST_CHECK_EQUAL( "foobar", generator.network_name() );
    BOOST_REQUIRE( session );
    BOOST_CHECK_EQUAL( session->session_id(), json::string( "1" ) );

    bayeux::session* same_session = connector.find_session( json::string( "1" ) );
    BOOST_CHECK( same_session == session );

    connector.idle_session( same_session );
}

/*!
 * @test drop session test
 */
BOOST_AUTO_TEST_CASE( drop_session_test )
{
    boost::asio::io_service queue;
    pubsub::test::adapter   adapter;
    pubsub::root            root( queue, adapter, pubsub::configuration() );

    test_session_generator  generator;
    connector_t             connector( queue, root, generator, bayeux::configuration() );

    bayeux::session* session = connector.create_session( "foobar" );
    connector.idle_session( session );

    BOOST_CHECK( session_alive( connector, "1" ) );
    connector.drop_session( json::string( "1" ) );
    BOOST_CHECK( !session_alive( connector, "1" ) );
}

/*!
 * @test drop a session, that is otherwise in use
 *
 * there are two possible behaviors:
 * - when dropping a session, mark the session as dropped and delete it, when the last outstanding use returns
 * - just ignore the dropping, if the session is currently in use and let the timeout cleanup
 */
BOOST_AUTO_TEST_CASE( drop_session_in_use )
{
    /*
    boost::asio::io_service queue;
    pubsub::test::adapter   adapter;
    pubsub::root            root( queue, adapter, pubsub::configuration() );

    test_session_generator  generator;
    connector_t             connector( queue, root, generator, bayeux::configuration() );

    bayeux::session* session = connector.create_session( "foobar" );
    connector.idle_session( session );

    session = connector.find_session( json::string( "1" ) );
    BOOST_REQUIRE( session );

    connector.drop_session( json::string( "1" ) );

    connector.idle_session( session );
    BOOST_CHECK( !session_alive( connector, "1" ) );
    */
}

/*!
 * @test after a configured session timeout the session must not be obtainable.
 */
BOOST_AUTO_TEST_CASE( session_timeout_test )
{
    boost::asio::io_service queue;
    pubsub::test::adapter   adapter;
    pubsub::root            root( queue, adapter, pubsub::configuration() );

    test_session_generator  generator;
    connector_t             connector( queue, root, generator,
        bayeux::configuration().session_timeout( boost::posix_time::seconds( 20u ) ) );

    bayeux::session* session = connector.create_session( "foobar" );
    connector.idle_session( session );

    advance_time( queue, 20u );

    BOOST_CHECK( connector.find_session( json::string( "1" ) ) == 0 );
}

/*!
 * @test test that the session doesn't timeout, when it is used regularly
 */
BOOST_AUTO_TEST_CASE( session_doesnt_get_timeout_when_used )
{
    boost::asio::io_service queue;
    pubsub::test::adapter   adapter;
    pubsub::root            root( queue, adapter, pubsub::configuration() );

    test_session_generator  generator;
    connector_t             connector( queue, root, generator,
        bayeux::configuration().session_timeout( boost::posix_time::seconds( 20u ) ) );

    bayeux::session* session = connector.create_session( "foobar" );
    connector.idle_session( session );

    advance_time( queue, 15u );
    BOOST_CHECK( session_alive( connector, "1" ) );

    advance_time( queue, 15u );
    BOOST_CHECK( session_alive( connector, "1" ) );

    advance_time( queue, 20u );
    BOOST_CHECK( !session_alive( connector, "1" ) );
}

/*!
 * @test test that a session doesn't timeout when it is in use
 */
BOOST_AUTO_TEST_CASE( session_in_use_doesnt_timeout )
{
    boost::asio::io_service queue;
    pubsub::test::adapter   adapter;
    pubsub::root            root( queue, adapter, pubsub::configuration() );

    test_session_generator  generator;
    connector_t             connector( queue, root, generator,
        bayeux::configuration().session_timeout( boost::posix_time::seconds( 100u ) ) );

    bayeux::session* session = connector.create_session( "foobar" );

    advance_time( queue, 120u );

    connector.idle_session( session );

    BOOST_CHECK( session_alive( connector, "1" ) );
}

/*!
 * @test session doesn't time out if used once
 */
BOOST_AUTO_TEST_CASE( single_outstanding_session_prevents_timeout_test )
{
    boost::asio::io_service queue;
    pubsub::test::adapter   adapter;
    pubsub::root            root( queue, adapter, pubsub::configuration() );

    test_session_generator  generator;
    connector_t             connector( queue, root, generator,
        bayeux::configuration().session_timeout( boost::posix_time::seconds( 20u ) ) );

    bayeux::session* session = connector.create_session( "foobar" );
    connector.idle_session( session );

    bayeux::session* second_handle = connector.find_session( json::string( "1" ) );

    advance_time( queue, 20u );
    BOOST_CHECK( session_alive( connector, "1" ) );

    connector.idle_session( second_handle );

    advance_time( queue, 20u );
    BOOST_CHECK( !session_alive( connector, "1" ) );
}

/*!
 * @test session will timeout independently from other sessions
 */
BOOST_AUTO_TEST_CASE( session_timeouts_are_independent )
{
    boost::asio::io_service queue;
    pubsub::test::adapter   adapter;
    pubsub::root            root( queue, adapter, pubsub::configuration() );

    test_session_generator  generator;
    connector_t             connector( queue, root, generator,
        bayeux::configuration().session_timeout( boost::posix_time::seconds( 5u ) ) );

    bayeux::session* sessionA = connector.create_session( "foobar" );
    connector.idle_session( sessionA );

    bayeux::session* sessionB = connector.create_session( "foobar" );

    advance_time( queue, 4u );
    connector.idle_session( sessionB );

    advance_time( queue, 1u );
    BOOST_CHECK( !session_alive( connector, "1" ) );
    BOOST_CHECK( session_alive( connector, "2" ) );

    advance_time( queue, 5u );
    BOOST_CHECK( !session_alive( connector, "2" ) );
}

/*!
 * @test used after timeout will cancel the timeout
 */
BOOST_AUTO_TEST_CASE( timeout_will_not_delete_session_if_in_use )
{
    boost::asio::io_service queue;
    pubsub::test::adapter   adapter;
    pubsub::root            root( queue, adapter, pubsub::configuration() );

    test_session_generator  generator;
    connector_t             connector( queue, root, generator,
        bayeux::configuration().session_timeout( boost::posix_time::seconds( 5u ) ) );

    bayeux::session* session = connector.create_session( "foobar" );
    connector.idle_session( session );

    // this will trigger the timeout call back, but will not execute the callback
    server::test::advance_time( boost::posix_time::seconds( 5u ) );
    session = connector.find_session( json::string( "1" ) );
    BOOST_CHECK( session );

    // but this will execute the timeout callback
    tools::run( queue );
    connector.idle_session( session );
}
