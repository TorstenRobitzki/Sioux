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
#include "server/test_tools.h"
#include "tools/asstring.h"

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
    bayeux::connector       connector( queue, root, generator, bayeux::configuration() );

    BOOST_CHECK( connector.find_session( json::string( "1" ) ).get() == 0 );

    boost::shared_ptr< bayeux::session > session = connector.create_session( "foobar" );
    connector.idle_session( session );

    BOOST_CHECK_EQUAL( "foobar", generator.network_name() );
    BOOST_REQUIRE( session.get() );
    BOOST_CHECK_EQUAL( session->session_id(), json::string( "1" ) );
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
    bayeux::connector       connector( queue, root, generator, bayeux::configuration() );

    boost::shared_ptr< bayeux::session > session = connector.create_session( "foobar" );
    connector.idle_session( session );

    connector.drop_session( json::string( "1" ) );
    BOOST_CHECK( connector.find_session( json::string( "1" ) ).get() == 0 );
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
    bayeux::connector       connector( queue, root, generator,
        bayeux::configuration().session_timeout( boost::posix_time::milliseconds( 100u ) ) );

    boost::shared_ptr< bayeux::session > session = connector.create_session( "foobar" );
    connector.idle_session( session );

    server::test::wait( boost::posix_time::microseconds( 120u ) );
    BOOST_CHECK( connector.find_session( json::string( "1" ) ).get() == 0 );
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
    bayeux::connector       connector( queue, root, generator,
        bayeux::configuration().session_timeout( boost::posix_time::milliseconds( 100u ) ) );

    boost::shared_ptr< bayeux::session > session = connector.create_session( "foobar" );
    connector.idle_session( session );

    server::test::wait( boost::posix_time::microseconds( 80u ) );

    session = connector.find_session( json::string( "1" ) );
    BOOST_CHECK( session.get() );
    connector.idle_session( session );

    server::test::wait( boost::posix_time::microseconds( 80u ) );
    session = connector.find_session( json::string( "1" ) );
    BOOST_CHECK( session.get() );
    connector.idle_session( session );

    server::test::wait( boost::posix_time::microseconds( 120u ) );
    session = connector.find_session( json::string( "1" ) );
    BOOST_CHECK( session.get() == 0 );
}

/*!
 * @test test that a session timeout doesn't when it is in use
 */
BOOST_AUTO_TEST_CASE( session_in_use_doesnt_timeout )
{
    boost::asio::io_service queue;
    pubsub::test::adapter   adapter;
    pubsub::root            root( queue, adapter, pubsub::configuration() );

    test_session_generator  generator;
    bayeux::connector       connector( queue, root, generator,
        bayeux::configuration().session_timeout( boost::posix_time::milliseconds( 100u ) ) );

    boost::shared_ptr< bayeux::session > session = connector.create_session( "foobar" );
    server::test::wait( boost::posix_time::microseconds( 120u ) );

    connector.idle_session( session );
    BOOST_CHECK( connector.find_session( json::string( "1" ) ).get() );
}

