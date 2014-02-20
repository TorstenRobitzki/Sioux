#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>
#include "server/test_traits.h"
#include "server/connection.h"
#include "asio_mocks/test_timer.h"
#include "asio_mocks/test_socket.h"
#include "asio_mocks/run.h"
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

        asio_mocks::response_t run( const json::value& obj, http::http_error_code error )
        {
            socket_t socket( queue, http::test::begin( http::test::simple_get_11 ), http::test::end( http::test::simple_get_11 ) );

            object_ = obj;
            error_  = error;

            std::vector< asio_mocks::response_t > response = asio_mocks::run( boost::posix_time::seconds( 20 ), socket, *this );
            BOOST_ASSERT( response.size() == 1u );

            return response.front();
        }

        json::value run_and_return_body( const json::value& obj )
        {
            const asio_mocks::response_t response = run( obj, http::http_ok );

            return json::parse( std::string( response.body.begin(), response.body.end() ) );
        }

        json::value object() const
        {
            return object_;
        }

        http::http_error_code error_code() const
        {
            return error_;
        }

        json::value             object_;
        http::http_error_code   error_;

        context()
            : object_( json::null() )
            , error_( http::http_ok )
        {
        }
    };

    template < class Trait, class Connection >
    boost::shared_ptr< server::async_response > response_factory::create_response(
        const boost::shared_ptr< Connection >&                    connection,
        const boost::shared_ptr< const http::request_header >&    header,
              Trait&                                              trait )
    {
        const context& cont = *static_cast< const context* >( static_cast< const void* >( &trait ) );
        const boost::shared_ptr< server::async_response > new_response(
            new json::response<Connection>(connection, cont.object(), cont.error_code()));

        return new_response;
    }

}

BOOST_FIXTURE_TEST_CASE( the_body_will_be_correctly_transported, context )
{
    const json::value no_empty_hash  = json::parse_single_quoted( "{ 'a': 1, 'b': [{}, 'qwerty'] }" );
    const json::value empty_hash     = json::parse_single_quoted( "{}" );
    const json::value no_empty_array = json::parse_single_quoted( "[{}, 'qwerty']" );

    BOOST_CHECK_EQUAL( no_empty_hash, run_and_return_body( no_empty_hash ) );
    BOOST_CHECK_EQUAL( empty_hash, run_and_return_body( empty_hash ) );
    BOOST_CHECK_EQUAL( no_empty_array, run_and_return_body( no_empty_array ) );
}

BOOST_FIXTURE_TEST_CASE( the_status_line_contains_the_right_code, context )
{
    BOOST_CHECK_EQUAL( run( json::array(), http::http_ok ).header->code(), http::http_ok );
    BOOST_CHECK_EQUAL( run( json::array(), http::http_moved_permanently ).header->code(), http::http_moved_permanently );
    BOOST_CHECK_EQUAL( run( json::array(), http::http_bad_request ).header->code(), http::http_bad_request );
    BOOST_CHECK_EQUAL( run( json::array(), http::http_bad_gateway ).header->code(), http::http_bad_gateway );
}

BOOST_FIXTURE_TEST_CASE( there_is_a_content_type_header, context )
{
    asio_mocks::response_t response = run( json::array(), http::http_ok );
    const http::header* content_type_header = response.header->find_header( "Content-Type" );

    BOOST_ASSERT( content_type_header );
    BOOST_CHECK_EQUAL( content_type_header->value(), "application/json" );

}


