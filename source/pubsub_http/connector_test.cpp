#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>
#include "pubsub_http/connector.h"
#include "pubsub/test_helper.h"
#include "pubsub/root.h"
#include "pubsub/configuration.h"
#include "http/response.h"
#include "http/http.h"
#include "json/json.h"
#include "asio_mocks/test_timer.h"
#include "asio_mocks/json_msg.h"

namespace {

    struct response
    {
        boost::shared_ptr< http::response_header > header;
    };

    struct context
    {
        // posts the given text as http message and returns the whole response
        response http_post( const char* json_msg )
        {
            return response();
        }

        json::object http_response( const char* json_msg )
        {
            return json::object();
        }

        boost::asio::io_service queue_;
        pubsub::test::adapter   adapter_;
        pubsub::root            data_;
        pubsub::http::connector< asio_mocks::timer > connector_;

        context()
            : queue_()
            , adapter_( queue_ )
            , data_( queue_, adapter_, pubsub::configuration() )
            , connector_( queue_, data_ )
        {
        }
    };

} // namespace

BOOST_FIXTURE_TEST_CASE( http_error_code_when_sending_a_empty_message, context )
{
//    BOOST_CHECK_EQUAL( http_post( "{}" ).header->code(), http::http_bad_request );
}

BOOST_FIXTURE_TEST_CASE( server_creates_session_id_with_first_message, context )
{
    const json::object response = http_response(
        "{"
        "   'cmd': [ { 'subscribe': { 'a':1 ,'b':2 }, 'version': 34 } ]"
        "}" );

//    BOOST_CHECK_EQUAL( response.at( json::string( "id" ) ), json::string( "/0" ) );
}
