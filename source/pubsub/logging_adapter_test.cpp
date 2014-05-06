#include <boost/test/unit_test.hpp>
#include "pubsub/logging_adapter.h"
#include "pubsub/test_helper.h"
#include "json/json.h"
#include <sstream>

namespace {

    struct context {
        std::ostringstream      output;
        pubsub::test::adapter   mock;
        pubsub::logging_adapter logging;
        pubsub::adapter&        adapter;

        context() 
            : output()
            , mock()
            , logging( mock, output )
            , adapter( logging )
        {}

        explicit context( std::ostream& user_provided_output ) 
            : output()
            , mock()
            , logging( mock, user_provided_output )
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

BOOST_FIXTURE_TEST_CASE( validate_node_will_be_logged, context )
{
    adapter.validate_node( node, validation_call_back );
    mock.answer_validation_request( node, true );

    BOOST_CHECK_EQUAL( 
        json::parse( output.str() + "]"), 
        json::parse_single_quoted( 
            "[{ 'call': 'validate_node', 'node': {'a': '1', 'b': '3'}, 'result': true }]" ) );
}

BOOST_FIXTURE_TEST_CASE( failed_validate_node_will_be_logged, context )
{
    mock.answer_validation_request( node, false );
    adapter.validate_node( node, validation_call_back );

    BOOST_CHECK_EQUAL( 
        json::parse( output.str() + "]"), 
        json::parse_single_quoted( 
            "[{ 'call': 'validate_node', 'node': {'a': '1', 'b': '3'}, 'result': false }]" ) );
}

BOOST_FIXTURE_TEST_CASE( skipped_validate_node_will_be_logged, context )
{
    mock.skip_validation_request( node );
    adapter.validate_node( node, validation_call_back );

    BOOST_CHECK_EQUAL( 
        json::parse( output.str() + "]"), 
        json::parse_single_quoted( 
            "[{ 'call': 'validate_node', 'node': {'a': '1', 'b': '3'}, 'result': false }]" ) );
}

BOOST_FIXTURE_TEST_CASE( node_initialization_will_be_logged, context )
{
    mock.answer_initialization_request( node, json::string( "initial Value") );
    adapter.node_init( node, initialization_call_back );

    BOOST_CHECK_EQUAL( 
        json::parse( output.str() + "]"), 
        json::parse_single_quoted( 
            "[{ 'call': 'node_init', 'node': {'a': '1', 'b': '3'}, 'result': 'initial Value' }]" ) );
}

BOOST_FIXTURE_TEST_CASE( failed_node_initialization_will_be_logged, context )
{
    mock.skip_initialization_request( node );
    adapter.node_init( node, initialization_call_back );

    BOOST_CHECK_EQUAL( 
        json::parse( output.str() + "]"), 
        json::parse_single_quoted( 
            "[{ 'call': 'node_init', 'node': {'a': '1', 'b': '3'} }]" ) );
}

BOOST_FIXTURE_TEST_CASE( node_authorization_will_be_logged, context )
{
    mock.answer_authorization_request( node, true );
    adapter.authorize( subscriber, node, authorization_call_back );

    BOOST_CHECK_EQUAL( 
        json::parse( output.str() + "]"), 
        json::parse_single_quoted( 
            "[{ 'call': 'authorize', 'node': {'a': '1', 'b': '3'}, 'result': true }]" ) );
}

BOOST_FIXTURE_TEST_CASE( failed_node_authorization_will_be_logged, context )
{
    adapter.authorize( subscriber, node, authorization_call_back );
    mock.answer_authorization_request( subscriber, node, false );

    BOOST_CHECK_EQUAL( 
        json::parse( output.str() + "]"), 
        json::parse_single_quoted( 
            "[{ 'call': 'authorize', 'node': {'a': '1', 'b': '3'}, 'result': false }]" ) );
}

BOOST_FIXTURE_TEST_CASE( skipped_node_authorization_will_be_logged, context )
{
    mock.skip_authorization_request( subscriber, node );
    adapter.authorize( subscriber, node, authorization_call_back );

    BOOST_CHECK_EQUAL( 
        json::parse( output.str() + "]"), 
        json::parse_single_quoted( 
            "[{ 'call': 'authorize', 'node': {'a': '1', 'b': '3'}, 'result': false }]" ) );
}

BOOST_FIXTURE_TEST_CASE( multiple_calls_are_logged_in_order, context )
{
    adapter.validate_node( node, validation_call_back );
    mock.answer_validation_request( node, true );

    mock.answer_authorization_request( node, true );
    adapter.authorize( subscriber, node, authorization_call_back );

    mock.answer_initialization_request( node, json::string( "initial Value") );
    adapter.node_init( node, initialization_call_back );

    BOOST_CHECK_EQUAL( 
        json::parse( output.str() + "]"),
        json::parse_single_quoted( 
            "["
            "   { 'call': 'validate_node', 'node': {'a': '1', 'b': '3'}, 'result': true },"                
            "   { 'call': 'authorize', 'node': {'a': '1', 'b': '3'}, 'result': true },"
            "   { 'call': 'node_init', 'node': {'a': '1', 'b': '3'}, 'result': 'initial Value' }"
            "]" ) );

}

BOOST_AUTO_TEST_CASE( destructor_adds_a_closing_brace )
{
    std::ostringstream output;

    {
        context c( output );
        c.adapter.validate_node( node, validation_call_back );
        c.mock.answer_validation_request( node, true );
    }

    BOOST_CHECK_EQUAL( 
        json::parse( output.str() ),
        json::parse_single_quoted( 
            "[{ 'call': 'validate_node', 'node': {'a': '1', 'b': '3'}, 'result': true }]" ) ); 
}

BOOST_AUTO_TEST_SUITE_END()
