#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>
#include "server/test_traits.h"
#include "server/connection.h"
#include "asio_mocks/test_timer.h"
#include "asio_mocks/test_socket.h"
#include "asio_mocks/run.h"
#include "asio_mocks/json_msg.h"
#include "json/json.h"
#include "json_handler/response.h"
#include "http/test_request_texts.h"

namespace {

    struct response_factory
    {
        response_factory() {}

        template < class T >
        explicit response_factory( const T& ) {}

        template < class Trait, class Connection >
        static boost::shared_ptr< server::async_response > create_response(
            const boost::shared_ptr< Connection >&                    connection,
            const boost::shared_ptr< const http::request_header >&    header,
                  Trait&                                              trait );
    };

    typedef server::test::traits< response_factory >            traits_t;
    typedef asio_mocks::socket< const char* >                   socket_t;
    typedef asio_mocks::timer                                   timer_t;
    typedef server::connection< traits_t, socket_t, timer_t >   connection_t;
    typedef json::response< connection_t >                      response_t;

    struct context : traits_t
    {
        boost::asio::io_service queue;

        std::vector< asio_mocks::response_t > run( const asio_mocks::read_plan& input )
        {
            socket_t socket( queue, input );

            return asio_mocks::run( boost::posix_time::seconds( 20 ), socket, *this );
        }

        asio_mocks::response_t run_single( const asio_mocks::read_plan& input )
        {
            const std::vector< asio_mocks::response_t > result = run( input );
            BOOST_ASSERT( result.size() == 1u );

            return result.front();
        }

        json::value run_single_and_return_body( const char* request )
        {
            const asio_mocks::response_t response = run_single(
                asio_mocks::read( request ) << asio_mocks::disconnect_read() );

            return json::parse( std::string( response.body.begin(), response.body.end() ) );
        }

        json::value run_single_and_return_body( const json::value& obj )
        {
            const asio_mocks::response_t response = run_single( asio_mocks::json_msg( obj) << asio_mocks::disconnect_read() );

            return json::parse( std::string( response.body.begin(), response.body.end() ) );
        }

        // Test-Handler that will be passed to the created response by response_factory::create_response()
        static std::pair< json::value, http::http_error_code > respond_to_request( const ::http::request_header& header, const json::value& body )
        {
            json::object result;
            result.add( json::string( "body" ), body );

            // if the body is an array with exactly one element, we will use this as response code.
            const std::pair< bool, json::array > body_as_array = body.try_cast< json::array >();

            const http::http_error_code response_code = ( body_as_array.first && body_as_array.second.length() == 1u )
                ? static_cast< http::http_error_code >( body_as_array.second.at( 0 ).upcast< json::number >().to_int() )
                : http::http_ok;

            return std::pair< json::value, http::http_error_code >( result, response_code );
        }
    };

    template < class Trait, class Connection >
    boost::shared_ptr< server::async_response > response_factory::create_response(
        const boost::shared_ptr< Connection >&                    connection,
        const boost::shared_ptr< const http::request_header >&    header,
              Trait&                                              )
    {
        const typename json::response< Connection >::handler_t handler = boost::bind( &context::respond_to_request, _1, _2 );
        const boost::shared_ptr< server::async_response > new_response(
            new json::response< Connection >( connection, header, handler ) );

        return new_response;
    }

}

BOOST_FIXTURE_TEST_CASE( without_request_body_a_null_will_be_passed_to_the_handler, context )
{
    BOOST_CHECK_EQUAL(
        run_single_and_return_body( http::test::simple_get_11 ),
        json::parse_single_quoted("{'body': null}") );
}

BOOST_FIXTURE_TEST_CASE( will_not_crash_if_there_is_a_bug_in_the_request, context )
{
    const char request_with_buggy_body[] =
        "POST / HTTP/1.1\r\n"
        "Host: web-sniffer.net\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 50\r\n"
        "\r\n"
        "{\"hallo\": 123456776";

    run( asio_mocks::read( request_with_buggy_body ) << asio_mocks::disconnect_read() );
}

BOOST_FIXTURE_TEST_CASE( will_not_crash_if_there_is_no_json_body, context )
{
    run( asio_mocks::read( http::test::simple_post ) << asio_mocks::disconnect_read() );
}

BOOST_FIXTURE_TEST_CASE( the_callbacks_response_code_will_be_used, context )
{
    const asio_mocks::response_t response = run_single(
        asio_mocks::json_msg( "[" + tools::as_string( static_cast< int >( http::http_method_not_allowed ) ) + "]" ) << asio_mocks::disconnect_read() );

    BOOST_CHECK_EQUAL( response.header->code(), http::http_method_not_allowed );
}

BOOST_FIXTURE_TEST_CASE( body_will_be_correctly_transported, context )
{
    BOOST_CHECK_EQUAL(
        run_single_and_return_body(
            json::parse_single_quoted( "[ {}, 'a', 'b', null, { 'a': 4 } ]" ) ),
        json::parse_single_quoted(
            "{ "
            "   'body': [ {}, 'a', 'b', null, { 'a': 4 } ]"
            "}" ) );
}


