#define BOOST_TEST_MAIN

#include "pubsub_http/connector.h"
#include "pubsub/test_helper.h"
#include "pubsub/root.h"
#include "pubsub/configuration.h"
#include "pubsub/node_group.h"
#include "http/response.h"
#include "http/http.h"
#include "http/decode_stream.h"
#include "http/test_request_texts.h"
#include "json/json.h"
#include "asio_mocks/test_timer.h"
#include "asio_mocks/json_msg.h"
#include "asio_mocks/run.h"
#include "server/test_traits.h"
#include "server/connection.h"
#include "server/test_session_generator.h"
#include "tools/io_service.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/test/unit_test.hpp>

namespace {

    struct response_factory
    {
        template < class Trait >
        explicit response_factory( Trait& trait )
          : connector( trait.connector_ )
        {
        }

        template < class Connection >
        boost::shared_ptr< server::async_response > create_response(
            const boost::shared_ptr< Connection >&                    connection,
            const boost::shared_ptr< const http::request_header >&    header )
        {
            boost::shared_ptr< server::async_response > result;

            if ( header->state() == http::message::ok )
            {
                result = connector.create_response( connection, header );
            }

            if ( !result.get() )
                result = error_response( connection, http::http_bad_request );

            return result;
        }

        template < class Connection >
        boost::shared_ptr< server::async_response > error_response(
            const boost::shared_ptr< Connection >&  con,
            http::http_error_code                   ec )
        {
            return boost::shared_ptr< server::async_response >( new ::server::error_response< Connection >( con, ec ) );
        }

        pubsub::http::connector< asio_mocks::timer >&   connector;
    };

    typedef server::connection_traits<
        asio_mocks::socket< const char*, asio_mocks::timer >,
        asio_mocks::timer,
        response_factory,
        server::null_event_logger > trait_t;
    typedef server::connection< trait_t >               connection_t;
    typedef trait_t::network_stream_type                socket_t;

    struct context : trait_t, boost::asio::io_service, pubsub::test::adapter
    {
        std::vector< asio_mocks::response_t > http_multiple_post( const asio_mocks::read_plan& simulated_input )
        {
            socket_t                            socket( *this, simulated_input );
            return asio_mocks::run( boost::posix_time::seconds( 40 ), socket, static_cast< trait_t& >( *this ) );
        }

        json::array json_multiple_post( const asio_mocks::read_plan& simulated_input )
        {
            std::vector< asio_mocks::response_t > http_response = http_multiple_post( simulated_input );

            json::array result;
            for ( std::vector< asio_mocks::response_t >::const_iterator http = http_response.begin(); http != http_response.end(); ++http )
            {
                if ( http->header->code() != http::http_ok )
                    throw std::runtime_error( "during json_multiple_post: http-response: " + tools::as_string( http->header->code() ) );

                result.add(
                    json::parse( http->body.begin(), http->body.end() ).upcast< json::object >() );
            }

            return result;
        }

        asio_mocks::response_t http_post( const asio_mocks::read_plan& simulated_input )
        {
            const std::vector< asio_mocks::response_t > http_answer = http_multiple_post( simulated_input );

            if ( http_answer.size() != 1 )
            {
                throw std::runtime_error( "expected exactly one response, got: " + tools::as_string( http_answer.size() ) );
            }

            return http_answer.front();
        }

        // posts the given text as http message and returns the whole response
        asio_mocks::response_t http_post_json_msg( const char* json_msg )
        {
            return http_post(
                asio_mocks::read_plan()
             << asio_mocks::json_msg( json_msg )
             << asio_mocks::disconnect_read() );
        }

        json::object json_body( const asio_mocks::response_t& response )
        {
            return json::parse( std::string( response.body.begin(), response.body.end() ) ).upcast< json::object >();
        }

        json::object json_post( const char* json_msg )
        {
            const asio_mocks::response_t response = http_post_json_msg( json_msg );
            return json_body( response );
        }

        pubsub::root                                            data_;
        server::test::session_generator                         session_generator_;
        mutable pubsub::http::connector< asio_mocks::timer >    connector_;
        const pubsub::node_name                                 node1;
        const pubsub::node_name                                 node2;

        context()
            : trait_t( *this )
            , boost::asio::io_service()
            , pubsub::test::adapter( static_cast< boost::asio::io_service& >( *this ) )
            , data_( *this, *this, pubsub::configuration() )
            , session_generator_()
            , connector_( *this, data_, session_generator_ )
            , node1( json::parse_single_quoted( "{ 'a': 1, 'b': 1 }" ).upcast< json::object >() )
            , node2( json::parse_single_quoted( "{ 'a': 1, 'b': 2 }" ).upcast< json::object >() )
        {
        }

        asio_mocks::read_plan subscribe_to_node1()
        {
            answer_validation_request( node1, true );
            answer_authorization_request( node1, true );
            answer_initialization_request( node1, json::number( 42 ) );

            return asio_mocks::read_plan()
                << asio_mocks::json_msg( "{ 'cmd': [ { 'subscribe': { 'a':1 ,'b':1 } } ] }" );
        }
    };

    bool find_update( const json::object& response, const char* node_name_str, const char* data_str )
    {
        const json::object node_name = json::parse_single_quoted( node_name_str ).upcast< json::object >();
        const json::value  data      = json::parse_single_quoted( data_str );

        const json::value* updates = response.find( json::string( "update" ) );
        BOOST_REQUIRE( updates );

        const std::pair< bool, json::array > updates_arr = updates->try_cast< json::array >();
        BOOST_REQUIRE( updates_arr.first );

        bool found = false;
        for ( std::size_t i = 0; !found && i != updates_arr.second.length(); ++i )
        {
            const std::pair< bool, json::object > as_obj = updates_arr.second.at( i ).try_cast< json::object >();
            BOOST_REQUIRE( as_obj.first );

            const json::value* const key_value  = as_obj.second.find( json::string( "key" ) );
            const json::value* const data_value = as_obj.second.find( json::string( "data" ) );

            found = key_value != 0 && data_value != 0 && *key_value == node_name && *data_value == data;
        }

        return found;
    }
} // namespace

BOOST_FIXTURE_TEST_CASE( request_without_body_is_a_bad_request, context )
{
    using namespace http::test;

    const asio_mocks::response_t resp = http_post(
           asio_mocks::read_plan()
        << asio_mocks::read( begin( simple_get_11 ), end( simple_get_11 ) )
        << asio_mocks::disconnect_read() );

    BOOST_CHECK_EQUAL( resp.header->code(), http::http_bad_request );
}

BOOST_FIXTURE_TEST_CASE( http_error_code_when_sending_an_empty_message, context )
{
    BOOST_CHECK_EQUAL( http_post_json_msg( "{}" ).header->code(), http::http_bad_request );
}

BOOST_FIXTURE_TEST_CASE( http_error_code_when_sending_an_array, context )
{
    BOOST_CHECK_EQUAL( http_post_json_msg( "['cmd']" ).header->code(), http::http_bad_request );
}

BOOST_FIXTURE_TEST_CASE( object_has_to_contain_only_valid_field_names, context )
{
    BOOST_CHECK_EQUAL( http_post_json_msg( "{ 'foo': 1 }" ).header->code(), http::http_bad_request );
    BOOST_CHECK_EQUAL( http_post_json_msg( "{ 'bar': 'asd' }" ).header->code(), http::http_bad_request );
    BOOST_CHECK_EQUAL( http_post_json_msg( "{ 'init': [] }" ).header->code(), http::http_bad_request );
}

BOOST_FIXTURE_TEST_CASE( object_has_to_contain_no_extra_fields, context )
{
    const asio_mocks::response_t response = http_post_json_msg(
        "{"
            "'cmd': [ { 'subscribe': 1 } ],"
            "'extra': 1 "
        "}");

    BOOST_CHECK_EQUAL( response.header->code(), http::http_bad_request );
}

BOOST_FIXTURE_TEST_CASE( if_list_of_commands_is_empty_a_session_id_must_be_given, context )
{
    BOOST_CHECK_EQUAL( http_post_json_msg( "{ 'cmd': [] }" ).header->code(), http::http_bad_request );
}

BOOST_FIXTURE_TEST_CASE( server_creates_session_id_with_first_message, context )
{
    const json::object response = json_post(
        "{"
        "   'cmd': [ { 'subscribe': { 'a':1 ,'b':2 }, 'version': 34 } ]"
        "}" );

    BOOST_REQUIRE( response.find( json::string( "id" ) ) );
    BOOST_CHECK_EQUAL( response.at( json::string( "id" ) ), json::string( "192.168.210.1:9999/0" ) );
}

BOOST_FIXTURE_TEST_CASE( server_will_responde_with_a_new_session_id_if_the_used_one_is_unknown, context )
{
    const json::object response = json_post(
        "{"
        "   'cmd': [ { 'subscribe': { 'a':1 ,'b':2 }, 'version': 34 } ],"
        "   'id': 4711 "
        "}" );

    BOOST_REQUIRE( response.find( json::string( "id" ) ) );
    BOOST_CHECK_EQUAL( response.at( json::string( "id" ) ), json::string( "192.168.210.1:9999/0" ) );
}

BOOST_FIXTURE_TEST_CASE( server_will_stick_to_the_session_id, context )
{
    const std::vector< asio_mocks::response_t > response = http_multiple_post(
           asio_mocks::read_plan()
        << asio_mocks::json_msg(
            "{"
            "   'cmd': [ { 'subscribe': { 'a':1 ,'b':2 }, 'version': 34 } ]"
            "}"  )
        << asio_mocks::json_msg(
            "{"
            "   'id': '192.168.210.1:9999/0' "
            "}" )
        << asio_mocks::disconnect_read() );

    BOOST_REQUIRE_EQUAL( response.size(), 2u );
    BOOST_CHECK_EQUAL( json_body( response[ 0 ] ).at( json::string( "id" ) ), json::string( "192.168.210.1:9999/0" ) );
    BOOST_CHECK_EQUAL( json_body( response[ 1 ] ).at( json::string( "id" ) ), json::string( "192.168.210.1:9999/0" ) );
}

BOOST_FIXTURE_TEST_CASE( server_refused_invalid_commands, context )
{
    BOOST_CHECK_EQUAL( http_post_json_msg( "{ 'cmd': [ {} ] }" ).header->code(), http::http_bad_request );
    BOOST_CHECK_EQUAL( http_post_json_msg( "{ 'cmd': [ { 'shutdow': true } ] }" ).header->code(), http::http_bad_request );
    BOOST_CHECK_EQUAL( http_post_json_msg(
        "{ "
        "   'cmd': [ { 'subscribe': { 'a':1 ,'b':2 } }, { 'shutdow': true } ]"
        " }" ).header->code(), http::http_bad_request );
}

BOOST_FIXTURE_TEST_CASE( the_node_name_of_an_subscribe_msg_has_to_be_an_object, context )
{
    const json::object response = json_post( "{ 'cmd': [ { 'subscribe': 1 } ] }" );

    BOOST_CHECK_EQUAL( response,
        json::parse_single_quoted(
            "{"
            "   'id': '192.168.210.1:9999/0',"
            "   'resp': [ { 'subscribe': 1, 'error': 'node name must be an object' } ]"
            "}" ) );
}

BOOST_FIXTURE_TEST_CASE( the_node_name_of_an_unsubscribe_msg_has_to_be_an_object, context )
{
    const json::object response = json_post( "{ 'cmd': [ { 'unsubscribe': 'abc' } ] }" );

    BOOST_CHECK_EQUAL( response,
        json::parse_single_quoted(
            "{"
            "   'id': '192.168.210.1:9999/0',"
            "   'resp': [ { 'unsubscribe': 'abc', 'error': 'node name must be an object' } ]"
            "}" ) );
}

BOOST_FIXTURE_TEST_CASE( a_node_name_must_not_be_empty, context )
{
    const json::object response = json_post( "{ 'cmd': [ { 'unsubscribe': {} } ] }" );

    BOOST_CHECK_EQUAL( response,
        json::parse_single_quoted(
            "{"
            "   'id': '192.168.210.1:9999/0',"
            "   'resp': [ { 'unsubscribe': {}, 'error': 'node name must not be empty' } ]"
            "}" ) );
}

BOOST_FIXTURE_TEST_CASE( a_new_session_gets_a_new_session_id, context )
{
    json_post( "{ 'cmd': [ { 'subscribe': { 'a':1 ,'b':2 } } ] }" );
    const json::object response = json_post( "{ 'cmd': [ { 'subscribe': { 'a':1 ,'b':2 } } ] }" );

    BOOST_REQUIRE( response.find( json::string( "id" ) ) );
    BOOST_CHECK_EQUAL( response.at( json::string( "id" ) ), json::string( "192.168.210.1:9999/1" ) );

}

BOOST_FIXTURE_TEST_CASE( after_30_seconds_a_session_will_be_deleted, context )
{
    json_post(
        "{"
        "   'cmd': [ { 'subscribe': { 'a':1 ,'b':2 }, 'version': 34 } ]"
        "}" );

    asio_mocks::advance_time( boost::posix_time::seconds( 31 ) );

    const json::object response = json_post(
        "{"
        "   'id': '192.168.210.1:9999/0' "
        "}" );

    BOOST_REQUIRE( response.find( json::string( "id" ) ) );
    BOOST_CHECK_EQUAL( response.at( json::string( "id" ) ), json::string( "192.168.210.1:9999/1" ) );
}

/**
 * @test the current behaviour is, that a first update to a subscription is delivered with the a second
 *       http request. Delivering the initial version of the data with the first http request would be
 *       fine too.
 */
BOOST_FIXTURE_TEST_CASE( response_to_subscription, context )
{
    answer_validation_request( node1, true );
    answer_authorization_request( node1, true );
    answer_initialization_request( node1, json::number( 42 ) );

    tools::run( *this );
    const json::array response = json_multiple_post(
           asio_mocks::read_plan()
        << asio_mocks::json_msg( "{ 'cmd': [ { 'subscribe': { 'a':1 ,'b':1 } } ] }" )
        << asio_mocks::json_msg( "{ 'id': '192.168.210.1:9999/0' }" )
        << asio_mocks::disconnect_read() );

    BOOST_REQUIRE_EQUAL( response.length(), 2u );
    json::array updates = response.at( 1 ).upcast< json::object >().at( json::string( "update" ) ).upcast< json::array >();

    BOOST_REQUIRE_EQUAL( updates.length(), 1u );
    json::object update = updates.at( 0 ).upcast< json::object >();

    BOOST_CHECK_EQUAL(
        update.at( json::string( "key" ) ),
        json::parse_single_quoted( "{ 'a':1 ,'b':1 }" ));

    BOOST_CHECK_EQUAL( update.at( json::string( "data" ) ), json::number( 42 ) );
}

static void update_node1_to_42( pubsub::test::adapter& adapter, const pubsub::node_name& node1 )
{
    adapter.answer_validation_request( node1, true );
    adapter.answer_authorization_request( node1, true );
    adapter.answer_initialization_request( node1, json::number( 42 ) );
}

BOOST_FIXTURE_TEST_CASE( defered_response_to_subscription_if_validation_was_asynchronous, context )
{
    const json::array responses = json_multiple_post(
           asio_mocks::read_plan()
        << asio_mocks::json_msg( "{ 'cmd': [ { 'subscribe': { 'a':1 ,'b':1 } } ] }" )
        << boost::bind( update_node1_to_42, boost::ref( static_cast< pubsub::test::adapter&>( *this ) ), node1 )
        << asio_mocks::json_msg( "{ 'id': '192.168.210.1:9999/0' }" )
        << asio_mocks::disconnect_read() );

    BOOST_REQUIRE_EQUAL( responses.length(), 2u );
    BOOST_CHECK( find_update( responses.at( 1u ).upcast< json::object >(), "{ 'a':1 ,'b':1 }", "42" ) );
}

BOOST_FIXTURE_TEST_CASE( error_message_if_subscription_subject_is_invalid, context )
{
    answer_validation_request( node1, false );

    const json::array response = json_multiple_post(
           asio_mocks::read_plan()
        << asio_mocks::json_msg( "{ 'cmd': [ { 'subscribe': { 'a':1 ,'b':1 } } ] }" )
        << asio_mocks::json_msg( "{ 'id': '192.168.210.1:9999/0' }" )
        << asio_mocks::disconnect_read() );

    BOOST_REQUIRE_EQUAL( response.length(), 2u );
    json::array resp = response.at( 1 ).upcast< json::object >().at( json::string( "resp" ) ).upcast< json::array >();

    BOOST_REQUIRE_EQUAL( resp.length(), 1u );

    BOOST_CHECK_EQUAL(
        resp.at( 0 ),
        json::parse_single_quoted( "{"
            "'error': 'invalid node'"
            "'subscribe' :{ 'a':1 ,'b':1 } }" ) );
}

static void invalidate_node_subject( pubsub::test::adapter& adapter, const pubsub::node_name& node )
{
    adapter.answer_validation_request( node, false );
}

BOOST_FIXTURE_TEST_CASE( defered_error_message_if_subscription_subject_is_invalid, context )
{
    const json::array response = json_multiple_post(
           asio_mocks::read_plan()
        << asio_mocks::json_msg( "{ 'cmd': [ { 'subscribe': { 'a':1 ,'b':1 } } ] }" )
        << asio_mocks::json_msg( "{ 'id': '192.168.210.1:9999/0' }" )
        << boost::bind( invalidate_node_subject, boost::ref( static_cast< pubsub::test::adapter&>( *this ) ), node1 )
        << asio_mocks::disconnect_read() );

    BOOST_REQUIRE_EQUAL( response.length(), 2u );
    json::array resp = response.at( 1 ).upcast< json::object >().at( json::string( "resp" ) ).upcast< json::array >();

    BOOST_REQUIRE_EQUAL( resp.length(), 1u );

    BOOST_CHECK_EQUAL(
        resp.at( 0 ),
        json::parse_single_quoted( "{"
            "'error': 'invalid node'"
            "'subscribe' :{ 'a':1 ,'b':1 } }" ) );
}

BOOST_FIXTURE_TEST_CASE( error_message_if_not_authorized, context )
{
    answer_validation_request( node1, true );
    answer_authorization_request( node1, false );

    const json::array response = json_multiple_post(
           asio_mocks::read_plan()
        << asio_mocks::json_msg( "{ 'cmd': [ { 'subscribe': { 'a':1 ,'b':1 } } ] }" )
        << asio_mocks::json_msg( "{ 'id': '192.168.210.1:9999/0' }" )
        << asio_mocks::disconnect_read() );

    BOOST_REQUIRE_EQUAL( response.length(), 2u );

    json::array resp = response.at( 1 ).upcast< json::object >().at( json::string( "resp" ) ).upcast< json::array >();

    BOOST_REQUIRE_EQUAL( resp.length(), 1u );

    BOOST_CHECK_EQUAL(
        resp.at( 0 ),
        json::parse_single_quoted( "{"
            "'error': 'not allowed'"
            "'subscribe' :{ 'a':1 ,'b':1 } }" ) );
}

static void unauthorized_node_subject( pubsub::test::adapter& adapter, const pubsub::node_name& node )
{
    adapter.answer_validation_request( node, true );
    adapter.answer_authorization_request( node, false );
}

BOOST_FIXTURE_TEST_CASE( defered_error_message_if_not_authorized, context )
{
    const json::array response = json_multiple_post(
           asio_mocks::read_plan()
        << asio_mocks::json_msg( "{ 'cmd': [ { 'subscribe': { 'a':1 ,'b':1 } } ] }" )
        << asio_mocks::json_msg( "{ 'id': '192.168.210.1:9999/0' }" )
        << boost::bind( unauthorized_node_subject, boost::ref( static_cast< pubsub::test::adapter&>( *this ) ), node1 )
        << asio_mocks::disconnect_read() );

    BOOST_REQUIRE_EQUAL( response.length(), 2u );
    json::array resp = response.at( 1 ).upcast< json::object >().at( json::string( "resp" ) ).upcast< json::array >();

    BOOST_REQUIRE_EQUAL( resp.length(), 1u );

    BOOST_CHECK_EQUAL(
        resp.at( 0 ),
        json::parse_single_quoted( "{"
            "'error': 'not allowed'"
            "'subscribe' :{ 'a':1 ,'b':1 } }" ) );
}

BOOST_FIXTURE_TEST_CASE( failed_initialization, context )
{
    answer_validation_request( node1, true );
    answer_authorization_request( node1, true );
    skip_initialization_request( node1 );

    const json::array response = json_multiple_post(
           asio_mocks::read_plan()
        << asio_mocks::json_msg( "{ 'cmd': [ { 'subscribe': { 'a':1 ,'b':1 } } ] }" )
        << asio_mocks::json_msg( "{ 'id': '192.168.210.1:9999/0' }" )
        << asio_mocks::disconnect_read() );

    BOOST_REQUIRE_EQUAL( response.length(), 2u );
    json::array resp = response.at( 1 ).upcast< json::object >().at( json::string( "resp" ) ).upcast< json::array >();

    BOOST_REQUIRE_EQUAL( resp.length(), 1u );

    BOOST_CHECK_EQUAL(
        resp.at( 0 ),
        json::parse_single_quoted( "{"
            "'error': 'node initialization failed'"
            "'subscribe' :{ 'a':1 ,'b':1 } }" ) );
}

static void update_node( pubsub::root& root, const pubsub::node_name& node, const json::value& val )
{
    root.update_node( node, val );
}

static void defered_update_node( pubsub::root& root, const pubsub::node_name& node,
    const json::value& val, boost::asio::io_service& queue )
{
    queue.post( boost::bind( update_node, boost::ref( root ), node, val ) );
}

BOOST_FIXTURE_TEST_CASE( getting_updates_while_waiting, context )
{
    json::array response = json_multiple_post(
           subscribe_to_node1()
        << asio_mocks::json_msg( "{ 'id': '192.168.210.1:9999/0' }" )
        << asio_mocks::json_msg( "{ 'id': '192.168.210.1:9999/0' }" )
        << boost::bind( update_node, boost::ref( data_ ), node1, json::string( "this is an update") )
        << asio_mocks::disconnect_read() );

    // set the version to a specific value 4
    response.at( 2u )
        .upcast< json::object >()
        .at( json::string( "update" ) )
        .upcast< json::array >()
        .at( 0 )
        .upcast< json::object >()
        .at( json::string( "version") ) = json::number( 4 );

    BOOST_CHECK_EQUAL( response.at( 2u ), json::parse_single_quoted(
        "{"
        "    'id': '192.168.210.1:9999/0',"
        "    'update': ["
        "        { "
        "            'key': { 'a': 1, 'b': 1 },"
        "            'data': 'this is an update',"
        "            'version': 4"
        "        }"
        "    ]"
        "}") );
}

BOOST_FIXTURE_TEST_CASE( getting_updates_before_polling, context )
{
    json::array response = json_multiple_post(
           subscribe_to_node1()
        << asio_mocks::json_msg( "{ 'id': '192.168.210.1:9999/0' }" )
        << boost::bind( defered_update_node, boost::ref( data_ ), node1, json::string( "update1"),
            boost::ref( static_cast< boost::asio::io_service& >( *this ) ) )
        << asio_mocks::json_msg( "{ 'id': '192.168.210.1:9999/0' }" )
        << asio_mocks::disconnect_read() );

    // set the version to a specific value 4, to make it better comparable
    response.at( 2u )
        .upcast< json::object >()
        .at( json::string( "update" ) )
        .upcast< json::array >()
        .at( 0 )
        .upcast< json::object >()
        .at( json::string( "version") ) = json::number( 4 );

    BOOST_CHECK_EQUAL( response.at( 2u ), json::parse_single_quoted(
        "{"
        "    'id': '192.168.210.1:9999/0',"
        "    'update': ["
        "        { "
        "            'key': { 'a': 1, 'b': 1 },"
        "            'data': 'update1',"
        "            'version': 4"
        "        }"
        "    ]"
        "}") );
}

BOOST_FIXTURE_TEST_CASE( updates_created_different_versions, context )
{
    const json::array response = json_multiple_post(
           subscribe_to_node1()
        << asio_mocks::json_msg( "{ 'id': '192.168.210.1:9999/0' }" )
        << boost::bind( defered_update_node, boost::ref( data_ ), node1, json::string( "update1"),
            boost::ref( static_cast< boost::asio::io_service& >( *this ) ) )
        << asio_mocks::json_msg( "{ 'id': '192.168.210.1:9999/0' }" )
        << asio_mocks::disconnect_read() );

    const json::value version1 = response.at( 1u )
        .upcast< json::object >()
        .at( json::string( "update" ) )
        .upcast< json::array >()
        .at( 0 )
        .upcast< json::object >()
        .at( json::string( "version") );

    const json::value version2 = response.at( 2u )
        .upcast< json::object >()
        .at( json::string( "update" ) )
        .upcast< json::array >()
        .at( 0 )
        .upcast< json::object >()
        .at( json::string( "version") );

    BOOST_CHECK_NE( version1, version2 );
}

BOOST_FIXTURE_TEST_CASE( unsubscribe_from_node, context )
{
    const json::array response = json_multiple_post(
           subscribe_to_node1()
        << asio_mocks::json_msg( "{"
            "   'id': '192.168.210.1:9999/0',  "
            "   'cmd': [ "
            "       { 'unsubscribe': { 'a': 1, 'b': 1 } } "
            "   ]"
            "}" )
        << boost::bind( defered_update_node, boost::ref( data_ ), node1, json::string( "update"),
            boost::ref( static_cast< boost::asio::io_service& >( *this ) ) )
        << asio_mocks::json_msg( "{ 'id': '192.168.210.1:9999/0' }" )
        << asio_mocks::json_msg( "{ 'id': '192.168.210.1:9999/0' }" )
        << asio_mocks::disconnect_read() );

    // the client was unsubscribed before the first update was received
    BOOST_CHECK_EQUAL(
        response,
        json::parse_single_quoted("["
            "   { 'id': '192.168.210.1:9999/0' },"
            "   { 'id': '192.168.210.1:9999/0' },"
            "   { 'id': '192.168.210.1:9999/0' },"
            "   { 'id': '192.168.210.1:9999/0' }"
            "]")
        );
}

BOOST_FIXTURE_TEST_CASE( unsubscribe_from_not_subscribed_node_http, context )
{
    const json::array response = json_multiple_post(
           subscribe_to_node1()
        << asio_mocks::json_msg( "{"
            "   'id': '192.168.210.1:9999/0',  "
            "   'cmd': [ "
            "       { 'unsubscribe': { 'a': 1, 'b': 2 } } "
            "   ]"
            "}" )
        << asio_mocks::disconnect_read() );

    BOOST_REQUIRE_EQUAL( response.length(), 2u );
    BOOST_CHECK_EQUAL(
        response.at( 1u ),
        json::parse_single_quoted(""
            "{"
            "   'id': '192.168.210.1:9999/0',"
            "   'resp': [{"
            "       'unsubscribe': { 'a': 1, 'b': 2 },"
            "       'error': 'not subscribed'"
            "   }]"
            "}" ) );
}

BOOST_FIXTURE_TEST_CASE( a_client_blocks_when_there_is_no_update, context )
{
    answer_validation_request( node1, true );
    answer_authorization_request( node1, true );
    answer_initialization_request( node1, json::number( 42 ) );

    const boost::posix_time::ptime start_time = asio_mocks::current_time();

    // the first message will always return immediately, the second will return
    // immediately, because it can transport the initial data of the subscribed node.
    // The third transport should block.
    const std::vector< asio_mocks::response_t > response = http_multiple_post(
           asio_mocks::read_plan()
        << asio_mocks::json_msg( "{ 'cmd': [ { 'subscribe': { 'a':1 ,'b':1 } } ] }" )
        << asio_mocks::json_msg( "{ 'id': '192.168.210.1:9999/0' }" )
        << asio_mocks::json_msg( "{ 'id': '192.168.210.1:9999/0' }" )
        << asio_mocks::disconnect_read() );

    const boost::posix_time::time_duration wait_time = response.back().received - start_time;

    BOOST_CHECK_GE( wait_time, boost::posix_time::seconds( 19 ) );
    BOOST_CHECK_LE( wait_time, boost::posix_time::seconds( 21 ) );
}

BOOST_FIXTURE_TEST_CASE( hurry_a_blocked_connection, context )
{
    const boost::posix_time::ptime start_time = asio_mocks::current_time();

    // the first message will always return immediately, the second will return
    // immediately, because it can transport the initial data of the subscribed node.
    // The third transport would block, if there is no third transport, that hurries the third one.
    // The last message doesn't use a valid session id, or otherwise two connection-detection would
    // cause the same effect.
    const std::vector< asio_mocks::response_t > response = http_multiple_post(
           asio_mocks::read_plan()
        << asio_mocks::json_msg( "{ 'cmd': [ { 'subscribe': { 'a':1 ,'b':1 } } ] }" )
        << asio_mocks::json_msg( "{ 'id': '192.168.210.1:9999/0' }" )
        << asio_mocks::json_msg( "{ 'id': '192.168.210.1:9999/0' }" )
        << asio_mocks::json_msg( "{ 'cmd': [ { 'subscribe': { 'a':1 ,'b':1 } } ] }" )
        << asio_mocks::disconnect_read() );

    BOOST_REQUIRE_GE( response.size(), 3u );

    const boost::posix_time::time_duration no_wait_time = response[ 2u ].received - start_time;

    BOOST_CHECK_LE( no_wait_time, boost::posix_time::seconds( 1 ) );
}

// durring tests, there was a situation, where after a browser refresh, the server did not respond to a subscription
BOOST_FIXTURE_TEST_CASE( second_subscription_to_invalid_node_must_be_communicated, context )
{
    /*
    data_.add_configuration( pubsub::node_group(), pubsub::configurator().authorization_not_required() );

    answer_validation_request( node1, false );

    // create a subscriber, subscribe to node1
    const boost::shared_ptr< pubsub::test::subscriber > first_subscriber( new pubsub::test::subscriber );
    data_.subscribe( first_subscriber, node1 );
    tools::run( *this );

    // make sure, that the subscriber is subscribed
    BOOST_REQUIRE( first_subscriber->on_invalid_node_subscription_called( node1 ) );

    const json::array response = json_multiple_post(
           asio_mocks::read_plan()
        << asio_mocks::json_msg( "{ 'cmd': [ { 'subscribe': { 'a':1 ,'b':1 } } ] }" )
        << asio_mocks::json_msg( "{ 'id': '192.168.210.1:9999/0' }" )
        << asio_mocks::disconnect_read() );

    BOOST_REQUIRE_GE( response.length(), 2u );

    // in the first, or the second response, there should be an update of node1
    BOOST_CHECK_NE( response.find( json::parse_single_quoted( "{ "
        "'id'     : '192.168.210.1:9999/0',"
        "'resp'   : [ { "
        "   'subscribe': {'a': 1, 'b': 1},"
        "   'error'    : 'invalid node' } ]"
    "}" ) ), -1 );
    */
}
