#ifndef SIOUX_SOURCE_PUBSUB_HTTP_TEST_CONTEXT_H
#define SIOUX_SOURCE_PUBSUB_HTTP_TEST_CONTEXT_H

#include "pubsub/test_helper.h"
#include "pubsub/root.h"
#include "pubsub/configuration.h"
#include "pubsub/node_group.h"
#include "pubsub/logging_adapter.h"
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
        std::vector< asio_mocks::response_t > http_multiple_post( const asio_mocks::read_plan& simulated_input, unsigned timeout_sec = 40 )
        {
            socket_t                            socket( *this, simulated_input );
            return asio_mocks::run( boost::posix_time::seconds( timeout_sec ), socket, static_cast< trait_t& >( *this ) );
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

        asio_mocks::response_t http_post( const asio_mocks::read_plan& simulated_input, unsigned timeout_sec = 40 )
        {
            const std::vector< asio_mocks::response_t > http_answer = http_multiple_post( simulated_input, timeout_sec );

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

        static const bool                                       log_adapter = false;
        std::ostream                                            dev_null_;
        pubsub::logging_adapter                                 adapter_logging_;
        pubsub::root                                            data_;
        server::test::session_generator                         session_generator_;
        mutable pubsub::http::connector< asio_mocks::timer >    connector_;
        const pubsub::node_name                                 node1;
        const pubsub::node_name                                 node2;

        context()
            : trait_t( *this )
            , boost::asio::io_service()
            , pubsub::test::adapter( static_cast< boost::asio::io_service& >( *this ) )
            , dev_null_( 0 )
            , adapter_logging_( static_cast< pubsub::adapter& >( *this ), log_adapter ? std::cout : dev_null_ )
            , data_( *this, log_adapter ? adapter_logging_ : static_cast< pubsub::adapter& >( *this ), pubsub::configuration() )
            , session_generator_()
            , connector_( *this, data_, session_generator_ )
            , node1( json::parse_single_quoted( "{ 'a': '1', 'b': '1' }" ).upcast< json::object >() )
            , node2( json::parse_single_quoted( "{ 'a': '1', 'b': '2' }" ).upcast< json::object >() )
        {
        }

        asio_mocks::read_plan subscribe_to_node1( const json::value& initial_value_for_node1 = json::number( 42 ) )
        {
            answer_validation_request( node1, true );
            answer_authorization_request( node1, true );
            answer_initialization_request( node1, initial_value_for_node1 );

            return asio_mocks::read_plan()
                << asio_mocks::json_msg( "{ 'cmd': [ { 'subscribe': { 'a': '1' ,'b': '1' } } ] }" );
        }

        asio_mocks::read_plan poll()
        {
            return asio_mocks::read_plan()
                << asio_mocks::json_msg( "{ 'id': '192.168.210.1:9999/0' }" );
        }

        json::object replace_in_update( json::value& update, const char* key, const json::value& new_val, int index = 0 )
        {
            json::object result = update.upcast< json::object >();

            result
                .at( json::string( "update" ) )
                .upcast< json::array >()
                .at( index )
                .upcast< json::object >()
                .at( key ) = new_val;

            return result;
        }

        json::object replace_version( json::value& org, const json::value& new_version, int index = 0 )
        {
            return replace_in_update( org, "version", new_version, index );
        }

        json::object replace_version( json::value& org, int new_version, int index = 0 )
        {
            return replace_version( org, json::number( new_version ), index );   
        }

        json::object replace_from( json::value& org, const json::value& new_version, int index = 0 )
        {
            return replace_in_update( org, "from", new_version, index );
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

#endif


