#include <boost/test/unit_test.hpp>
#include "pubsub/logging_adapter.h"
#include "pubsub/test_helper.h"
#include "json/json.h"

namespace {
    struct context {
        pubsub::test::adapter   mock;
        pubsub::logging_adapter logging;
        pubsub::adapter&        adapter;

        context() 
            : mock()
            , logging( mock )
            , adapter( logging )
        {}
    };

    pubsub::node_name                                       node( json::parse_single_quoted( "{'a': 1, 'b': 3}" ).upcast< json::object >() );
    boost::shared_ptr< pubsub::validation_call_back >       validation_call_back( new pubsub::test::validation_call_back );
    boost::shared_ptr< pubsub::authorization_call_back >    authorization_call_back( new pubsub::test::authorization_call_back );
    boost::shared_ptr< pubsub::subscriber >                 subscriber( new pubsub::test::subscriber );
    boost::shared_ptr< pubsub::initialization_call_back >   initialization_call_back( new pubsub::test::initialization_call_back );
}

BOOST_AUTO_TEST_SUITE( logging_adapter )

BOOST_FIXTURE_TEST_CASE( validate_node_calls_are_forwared, context )
{
    adapter.validate_node( node, validation_call_back );

    BOOST_CHECK( mock.validation_requested( node ) );
}

BOOST_FIXTURE_TEST_CASE( authorize_calls_are_forwared, context )
{
    adapter.authorize( subscriber, node, authorization_call_back );

    BOOST_CHECK( mock.authorization_requested( subscriber, node ) );
}

BOOST_FIXTURE_TEST_CASE( node_init_calls_are_forwared, context )
{
    adapter.node_init( node, initialization_call_back );

    BOOST_CHECK( mock.initialization_requested( node ) );
}

BOOST_FIXTURE_TEST_CASE( invalid_node_subscription_calls_are_forwared, context )
{
    adapter.invalid_node_subscription( node, subscriber );

    BOOST_CHECK( mock.invalid_node_subscription_reported( node, subscriber ) );
}

BOOST_FIXTURE_TEST_CASE( unauthorized_subscription_calls_are_forwared, context )
{
    adapter.unauthorized_subscription( node, subscriber );

    BOOST_CHECK( mock.unauthorized_subscription_reported( node, subscriber ) );
}

BOOST_FIXTURE_TEST_CASE( initialization_failed_calls_are_forwared, context )
{
    adapter.initialization_failed( node );

    BOOST_CHECK( mock.initialization_failed_reported( node ) );
}

BOOST_AUTO_TEST_SUITE_END()
