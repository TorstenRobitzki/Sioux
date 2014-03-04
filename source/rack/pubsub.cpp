#include <ruby.h>
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

        virtual std::vector< char > call( const std::vector< char >& body, const http::request_header& request,
            const boost::asio::ip::tcp::endpoint& );

        // a pointer is used to destroy the queue_ and thus the contained response-objects before the server and
        // thus accessed objects like the logging-trait
        boost::asio::io_service*            queue_;
        rack::ruby_land_queue               ruby_land_queue_;
        rack::adapter                       adapter_;
        pubsub::root                        data_;
        pubsub::http::connector<>           connector_;
        server_t                            server_;
        VALUE                               application_;

    };

    pubsub_server::pubsub_server( VALUE application, VALUE ruby_self, VALUE configuration )
        : queue_( new boost::asio::io_service )
        , adapter_( rb_hash_lookup( configuration, rb_str_new2( "Adapter" ) ), ruby_land_queue_ )
        , data_( *queue_, adapter_, pubsub_config( configuration ) )
        , connector_( *queue_, data_ )
        , server_( *queue_, 0, std::cout )
        , application_( application )
    {
        server_.add_action( "/pubsub", boost::bind( &pubsub_server::on_pubsub_request, this, _1, _2 ) );
        server_.add_action( "/publish", boost::bind( &pubsub_server::on_publish_request, this, _1, _2 ) );
        server_.add_action( "/", boost::bind( &pubsub_server::on_request, this, _1, _2 ) );

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

    boost::shared_ptr< server::async_response > pubsub_server::on_pubsub_request(
        const boost::shared_ptr< connection_t >& connection, const boost::shared_ptr< const http::request_header >& header )
    {
        const boost::shared_ptr< server::async_response > response = connector_.create_response( connection, header );

        return response.get()
            ? response
            : connection->trait().error_response( connection, http::http_bad_request );
    }

    boost::shared_ptr< server::async_response > pubsub_server::on_publish_request( const boost::shared_ptr< connection_t >&, const boost::shared_ptr< const http::request_header >& )
    {
        return boost::shared_ptr< server::async_response >();
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
    const pubsub::node_name node_name  = hash_to_node( node );
    const json::value       node_value = ruby_to_json( value, node_name );

    LOG_DETAIL( log_context << "update: " << node_name << " to " << node_value );

    pubsub_server* server_ptr = 0;
    Data_Get_Struct( self, pubsub_server, server_ptr );
    assert( server_ptr );

    server_ptr->update_node( node_name, node_value );

    return self;
}

extern "C" VALUE run_pubsub( VALUE self, VALUE application, VALUE configuration )
{
    VALUE result = Qfalse;

    try
    {
        logging::add_output( std::cout );
        LOG_INFO( rack::log_context << "starting pubsub_server...." );

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
