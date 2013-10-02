#include <boost/test/unit_test.hpp>
#include "pubsub_http/sessions.h"
#include "server/test_session_generator.h"
#include "asio_mocks/test_timer.h"
#include "tools/io_service.h"

namespace {

    struct context
        : boost::asio::io_service
        , server::test::session_generator
        , pubsub::http::sessions< asio_mocks::timer >
    {
        context()
            : boost::asio::io_service()
            , server::test::session_generator()
            , pubsub::http::sessions< asio_mocks::timer >( *this, *this )
            , default_id( "net/0" )
            , default_network( "net" )
        {
        }

        const json::string default_id;
        const std::string  default_network;
    };
}

BOOST_FIXTURE_TEST_CASE( accessing_a_session_perodical_within_the_timeout_period, context )
{
    for ( int times = 0; times != 10; ++times )
    {
        {
            pubsub::http::session session( *this, default_id, default_network );

            BOOST_CHECK_EQUAL( session.id(), json::string( "net/0" ) );
        }

        asio_mocks::advance_time( boost::posix_time::seconds( 29u ) );
        tools::run( *this );
    }
}

BOOST_FIXTURE_TEST_CASE( accessing_a_session_perodical_after_the_timeout_period, context )
{
    json::string not_expected;

    for ( int times = 0; times != 10; ++times )
    {
        {
            pubsub::http::session session( *this, default_id, default_network );

            BOOST_CHECK_NE( session.id(), not_expected );
            not_expected = session.id();
        }

        asio_mocks::advance_time( boost::posix_time::seconds( 31u ) );
        tools::run( *this );
    }
}
