#include <ruby.h>
#include <boost/thread/future.hpp>
#include <boost/lexical_cast.hpp>

#include "rack/log.h"
#include "rack/ruby_tools.h"
#include "rack/ruby_land_queue.h"
#include "rack/call_rack_application.h"
#include "rack/adapter.h"
#include "rack/response.inc"
#include "pubsub/pubsub.h"
#include "pubsub/node.h"
#include "pubsub/root.h"
#include "pubsub/configuration.h"
#include "pubsub_http/connector.h"
#include "server/server.h"
#include "tools/exception_handler.h"
#include "json_handler/response.h"

using namespace rack;

namespace {

    typedef
#       ifdef NDEBUG
            server::logging_server<
                server::null_event_logger,
                server::null_error_logger >
#       else
            server::logging_server<>
#       endif
            server_t;

    class pubsub_server : application_interface
    {
    public:
        pubsub_server( VALUE application, VALUE ruby_self, VALUE configuration );
        ~pubsub_server();

        void update_node( const pubsub::node_name& node_name, const json::value& new_data );

        void run();

        /**
         * @brief the pubsub protocol does not define a publish callback, as bayeux do. But for easier exchange of both
         *        protocols, this rack implementation will provide a publish callback.
         */
        std::pair< json::value, http::http_error_code > publish( const ::http::request_header& header, const json::value& body );

        typedef server_t::connection_t connection_t;
    private:
        void run_queue();

        boost::shared_ptr< server::async_response > on_pubsub_request( const boost::shared_ptr< connection_t >&, const boost::shared_ptr< const http::request_header >& );
        boost::shared_ptr< server::async_response > on_publish_request( const boost::shared_ptr< connection_t >&, const boost::shared_ptr< const http::request_header >& );
        boost::shared_ptr< server::async_response > on_request( const boost::shared_ptr< connection_t >&, const boost::shared_ptr< const http::request_header >& );

        // application_interface implementation
        virtual std::vector< char > call( const std::vector< char >& body, const http::request_header& request,
            const boost::asio::ip::tcp::endpoint& );

        typedef rack::adapter::pubsub_publish_result_t publish_result_t;

        // function to be called on the C++ server side, delegating the ruby upcall to the ruby_land_queue
        publish_result_t publish_request_proxy( const ::http::request_header& header, const json::value& body );

        // converting arguments and return values of the upcall to ruby land
        void publish_request_impl( boost::promise< publish_result_t >& result, const json::value&, application_interface& );


        // a pointer is used to destroy the queue_ and thus the contained response-objects before the server and
        // thus accessed objects like the logging-trait
        boost::asio::io_service*            queue_;
        rack::ruby_land_queue               ruby_land_queue_;
        rack::adapter                       adapter_;
        pubsub::root                        data_;
        pubsub::http::connector<>           connector_;
        server_t                            server_;
        VALUE                               application_;
        VALUE                               self_;

    };

    pubsub_server::pubsub_server( VALUE application, VALUE ruby_self, VALUE configuration )
        : queue_( new boost::asio::io_service )
        , adapter_( rb_hash_lookup( configuration, rb_str_new2( "Adapter" ) ), ruby_land_queue_ )
        , data_( *queue_, adapter_, pubsub_config( configuration ) )
        , connector_( *queue_, data_ )
        , server_( *queue_, 0, std::cout )
        , application_( application )
        , self_( ruby_self )
    {
        server_.add_action( "/pubsub", boost::bind( &pubsub_server::on_pubsub_request, this, _1, _2 ) );
        server_.add_action( "/publish", boost::bind( &pubsub_server::on_publish_request, this, _1, _2 ) );
        server_.add_action( "/", boost::bind( &pubsub_server::on_request, this, _1, _2 ) );

        const unsigned timeout = rack::from_hash( configuration, "Sioux.timeout" );
        server_.trait().timeout( boost::posix_time::seconds( timeout ) );

        const unsigned port = rack::from_hash( configuration, "Port" );

        using namespace boost::asio::ip;
        server_.add_listener( tcp::endpoint( address( address_v4::any() ), port ) );
    }

    pubsub_server::~pubsub_server()
    {
        delete queue_;
    }

    void pubsub_server::update_node( const pubsub::node_name& node_name, const json::value& new_data )
    {
        data_.update_node( node_name, new_data );
    }

    // wraps an exception handler around io_service::run()
    void pubsub_server::run_queue()
    {
        for ( ;; )
        {
            try
            {
                queue_->run();
                break;
            }
            catch ( ... )
            {
                LOG_ERROR( rack::log_context << "in pubsub_server::run_queue(): "  << tools::exception_text() );
            }
        }
    }

    typedef std::pair< boost::thread*, server_t* > join_data_t;

    extern "C" VALUE pubsub_join_threads( void* arg )
    {
        join_data_t *const threads = static_cast< join_data_t* >( arg );
        threads->first->join();
        threads->second->join();

        return VALUE();
    }

    void pubsub_server::run()
    {
        boost::thread queue_runner( boost::bind( &pubsub_server::run_queue, this ) );

        ruby_land_queue_.process_request( *this );
        server_.shut_down();

        join_data_t joindata( &queue_runner, &server_ );
        rb_thread_blocking_region( &pubsub_join_threads, &joindata, 0, 0 );
    }

    std::vector< char > pubsub_server::call( const std::vector< char >& body, const http::request_header& request,
        const boost::asio::ip::tcp::endpoint& endpoint )
    {
        return rack::call_rack_application( body, request, endpoint, application_, ruby_land_queue_ );
    }

    std::pair< json::value, http::http_error_code > pubsub_server::publish_request_proxy( const ::http::request_header&, const json::value& body )
    {
        LOG_MAIN( log_context << "pubsub_server::publish_request_proxy(" << body << ")" );
        boost::promise< publish_result_t > result;

        rack::ruby_land_queue::call_back_t ruby_execution(
            boost::bind( &pubsub_server::publish_request_impl, this, boost::ref( result ), boost::cref( body ), _1 ) );

        ruby_land_queue_.push( ruby_execution );

        const publish_result_t& future_result = result.get_future().get();   
        LOG_MAIN( log_context << "pubsub_server::publish_request_proxy: " << future_result.first << ":" << future_result.second );

        return future_result;
    }

    void pubsub_server::publish_request_impl( boost::promise< publish_result_t >& result, const json::value& value, application_interface& )
    {
        result.set_value( adapter_.publish( value, self_ ) );
    }

    boost::shared_ptr< server::async_response > pubsub_server::on_pubsub_request(
        const boost::shared_ptr< connection_t >& connection, const boost::shared_ptr< const http::request_header >& header )
    {
        const boost::shared_ptr< server::async_response > response = connector_.create_response( connection, header );

        return response.get()
            ? response
            : connection->trait().error_response( connection, http::http_bad_request );
    }

    boost::shared_ptr< server::async_response > pubsub_server::on_publish_request(
        const boost::shared_ptr< connection_t >&                connection,
        const boost::shared_ptr< const http::request_header >&  request )
    {
        return boost::shared_ptr< server::async_response >(
            new json::response< connection_t >( connection, request, boost::bind( &pubsub_server::publish_request_proxy, this, _1, _2 ) ) );
    }

    boost::shared_ptr< server::async_response > pubsub_server::on_request(
        const boost::shared_ptr< connection_t >& connection, const boost::shared_ptr< const http::request_header >& request )
    {
        const boost::shared_ptr< server::async_response > result(
            new rack::response< connection_t >( connection, request, *queue_, ruby_land_queue_ ) );

        return result;
    }

}

extern "C" VALUE update_node_pubsub( VALUE self, VALUE node, VALUE value )
{
    LOG_MAIN( log_context << "update_node_pubsub: " );
    const pubsub::node_name node_name  = hash_to_node( node );
    const json::value       node_value = ruby_to_json( value, node_name );
    LOG_MAIN( log_context << node_name << " to " << node_value );

    pubsub_server* server_ptr = 0;
    Data_Get_Struct( self, pubsub_server, server_ptr );
    assert( server_ptr );

    server_ptr->update_node( node_name, node_value );

    LOG_DETAIL( log_context << "update_node_pubsub()." );
    return self;
}

static void configure_logging( VALUE configuration )
{
    logging::add_output( std::cout );
    LOG_INFO( rack::log_context << "starting pubsub_server...." );

    const logging::log_level pubsub_output_level = boost::lexical_cast< logging::log_level >( str_from_hash( configuration, "Loglevel.pubsub" ) );
    LOG_INFO( rack::log_context << "setting log level for pubsub to: " << pubsub_output_level );
    logging::set_level( rack::log_context, pubsub_output_level );
}

extern "C" VALUE run_pubsub( VALUE self, VALUE application, VALUE configuration )
{
    VALUE result = Qfalse;

    try
    {
        configure_logging( configuration );
        pubsub_server server( application, self, configuration );

        rack::local_data_ptr local_ptr( self, server );
        server.run();

        result = Qtrue;
    }
    catch ( const std::exception& e )
    {
        rb_raise( rb_eRuntimeError, "exception calling Rack::Handler::Sioux.run(): %s", e.what() );
    }
    catch ( ... )
    {
        rb_raise( rb_eRuntimeError, "unknown exception calling Rack::Handler::Sioux.run()" );
    }

    return result;
}

extern "C" VALUE alloc_pubsub( VALUE klass )
{
    return Data_Wrap_Struct( klass, 0, 0, 0 );
}

extern "C" void Init_pubsub_sioux()
{
    const VALUE mod_sioux = rb_define_module_under( rb_define_module( "Rack" ), "Sioux" );
    VALUE class_    = rb_define_class_under( mod_sioux, "SiouxPubsubImplementation", rb_cObject );

    rb_define_alloc_func( class_, alloc_pubsub );
    rb_define_method( class_, "run", RUBY_METHOD_FUNC( run_pubsub ), 2 );
    rb_define_method( class_, "[]=", RUBY_METHOD_FUNC( update_node_pubsub ), 2 );
}
