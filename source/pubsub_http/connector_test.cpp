#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>
#include "pubsub_http/connector.h"
#include "pubsub/test_helper.h"
#include "pubsub/root.h"
#include "pubsub/configuration.h"
#include "http/response.h"
#include "http/http.h"
#include "http/decode_stream.h"
#include "http/test_request_texts.h"
#include "json/json.h"
#include "asio_mocks/test_timer.h"
#include "asio_mocks/json_msg.h"
#include "server/test_traits.h"
#include "server/connection.h"
#include "server/test_session_generator.h"
#include "tools/io_service.h"

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

    struct response_t
    {
        boost::shared_ptr< http::response_header >  header;
        std::vector< char >                         body;
    };

    typedef server::connection_traits<
        asio_mocks::socket< const char*, asio_mocks::timer >,
        asio_mocks::timer,
        response_factory,
        server::null_event_logger > trait_t;
    typedef server::connection< trait_t >               connection_t;
    typedef trait_t::network_stream_type                socket_t;

    struct context : trait_t
    {
        std::vector< response_t > http_multiple_post( const asio_mocks::read_plan& simulated_input )
        {
            socket_t                            socket( queue_, simulated_input );
            boost::shared_ptr< connection_t >   http_connection( new connection_t( socket, *this ) );

            http_connection->start();
            tools::run( queue_ );

            typedef std::vector< std::pair< boost::shared_ptr< http::response_header >, std::vector< char > > > response_list_t;
            const response_list_t http_answer = http::decode_stream< http::response_header >( socket.bin_output() );

            std::vector< response_t > result;
            for ( response_list_t::const_iterator resp = http_answer.begin(); resp != http_answer.end(); ++resp )
            {
                response_t response = { resp->first, resp->second };
                result.push_back( response );
            }

            return result;
        }

        json::array json_multiple_post( const asio_mocks::read_plan& simulated_input )
        {
            std::vector< response_t > http_response = http_multiple_post( simulated_input );

            json::array result;
            for ( std::vector< response_t >::const_iterator http = http_response.begin(); http != http_response.end(); ++http )
            {
                if ( http->header->code() != http::http_ok )
                    throw std::runtime_error( "during json_multiple_post: http-response: " + tools::as_string( http->header->code() ) );

                result.add(
                    json::parse( http->body.begin(), http->body.end() ).upcast< json::object >() );
            }

            return result;
        }

        response_t http_post( const asio_mocks::read_plan& simulated_input )
        {
            const std::vector< response_t > http_answer = http_multiple_post( simulated_input );

            if ( http_answer.size() != 1 )
            {
                throw std::runtime_error( "expected exactly one response, got: " + tools::as_string( http_answer.size() ) );
            }

            return http_answer.front();
        }

        // posts the given text as http message and returns the whole response
        response_t http_post_json_msg( const char* json_msg )
        {
            return http_post(
                asio_mocks::read_plan()
             << asio_mocks::json_msg( json::parse_single_quoted( json_msg ).to_json() )
             << asio_mocks::disconnect_read() );
        }

        json::object json_post( const char* json_msg )
        {
            response_t response = http_post_json_msg( json_msg );
            return json::parse( response.body.begin(), response.body.end() ).upcast< json::object >();
        }

        boost::asio::io_service queue_;
        pubsub::test::adapter   adapter_;
        pubsub::root            data_;
        server::test::session_generator                      session_generator_;
        mutable pubsub::http::connector< asio_mocks::timer > connector_;

        context()
            : trait_t( *this )
            , queue_()
            , adapter_( queue_ )
            , data_( queue_, adapter_, pubsub::configuration() )
            , session_generator_()
            , connector_( queue_, data_, session_generator_ )
        {
        }
    };

/*
    json::array extract_cmd_responses( const json::value& response )
    {
        const std::pair< bool, json::object > response_as_obj = response.try_cast< json::object >();
        BOOST_REQUIRE( response_as_obj.first );

        const json::value* cmds = response_as_obj.second.find( json::string( "resp" ) );
        BOOST_REQUIRE( cmds );

        const std::pair< bool, json::array > cmd_list = cmds->try_cast< json::array >();
        BOOST_REQUIRE( cmd_list.first );

        return cmd_list.second;
    }

    json::object extract_cmd_response( const json::value& response, std::size_t index = 0 )
    {
        const json::array list = extract_cmd_responses( response );
        BOOST_REQUIRE( list.length() > index );

        return list.at( index );
    }
*/
} // namespace

BOOST_FIXTURE_TEST_CASE( request_without_body_is_a_bad_request, context )
{
    using namespace http::test;

    const response_t resp = http_post(
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
    const response_t response = http_post_json_msg(
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
    json_post(
        "{"
        "   'cmd': [ { 'subscribe': { 'a':1 ,'b':2 }, 'version': 34 } ]"
        "}" );

    const json::object response = json_post(
        "{"
        "   'id': '192.168.210.1:9999/0' "
        "}" );

    BOOST_REQUIRE( response.find( json::string( "id" ) ) );
    BOOST_CHECK_EQUAL( response.at( json::string( "id" ) ), json::string( "192.168.210.1:9999/0" ) );
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
