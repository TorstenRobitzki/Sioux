#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>
#include "pubsub_http/connector.h"
#include "pubsub/test_helper.h"
#include "pubsub/root.h"
#include "pubsub/configuration.h"
#include "http/response.h"
#include "http/http.h"
#include "http/decode_stream.h"
#include "json/json.h"
#include "asio_mocks/test_timer.h"
#include "asio_mocks/json_msg.h"
#include "server/test_traits.h"
#include "server/connection.h"
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
            if ( header->state() == http::message::ok )
            {
                return connector.create_response( connection, header );
            }

            return boost::shared_ptr< server::async_response >(
                    new server::error_response< Connection >( connection, http::http_bad_request ) );
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
        boost::shared_ptr< http::response_header > header;
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
        response_t http_post( const asio_mocks::read_plan& simulated_input )
        {
            socket_t                            socket( queue_, simulated_input );
            boost::shared_ptr< connection_t >   http_connection( new connection_t( socket, *this ) );

            http_connection->start();
            tools::run( queue_ );

            const std::vector< std::pair< boost::shared_ptr< http::response_header >, std::vector< char > > > http_answer =
                http::decode_stream< http::response_header >( socket.bin_output() );

            if ( http_answer.size() != 1 )
                throw std::runtime_error( "expected exactly one response, got: " + tools::as_string( http_answer.size() ) );

            const response_t result = { http_answer.front().first };

            return result;
        }

        // posts the given text as http message and returns the whole response
        response_t http_post( const char* json_msg )
        {
            return http_post(
                asio_mocks::read_plan() << asio_mocks::read( json::parse_single_quoted( json_msg ).to_json() ) );
        }

        json::object http_response( const char* json_msg )
        {
            return json::object();
        }

        boost::asio::io_service queue_;
        pubsub::test::adapter   adapter_;
        pubsub::root            data_;
        mutable pubsub::http::connector< asio_mocks::timer > connector_;

        context()
            : trait_t( *this )
            , queue_()
            , adapter_( queue_ )
            , data_( queue_, adapter_, pubsub::configuration() )
            , connector_( queue_, data_ )
        {
        }
    };

} // namespace

BOOST_FIXTURE_TEST_CASE( http_error_code_when_sending_a_empty_message, context )
{
    BOOST_CHECK_EQUAL( http_post( "{}" ).header->code(), http::http_bad_request );
}

BOOST_FIXTURE_TEST_CASE( server_creates_session_id_with_first_message, context )
{
    const json::object response = http_response(
        "{"
        "   'cmd': [ { 'subscribe': { 'a':1 ,'b':2 }, 'version': 34 } ]"
        "}" );

//    BOOST_CHECK_EQUAL( response.at( json::string( "id" ) ), json::string( "/0" ) );
}



