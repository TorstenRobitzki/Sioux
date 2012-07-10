// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.


#include <ruby.h>
#include <assert.h>
#include <iostream>
#include <boost/bind.hpp>

#include "rack/response.h"
#include "rack/response.inc"
#include "rack/ruby_tools.h"
#include "server/server.h"
#include "server/secure_session_generator.h"
#include "bayeux/bayeux.h"
#include "bayeux/log.h"
#include "pubsub/pubsub.h"
#include "pubsub/configuration.h"
#include "tools/split.h"
#include "tools/iterators.h"


namespace
{

    typedef
#       ifdef NDEBUG
            server::logging_server<
                bayeux::stream_event_log< server::null_event_logger >,
                server::null_error_logger >
#       else
            server::logging_server<>
#       endif
            server_t;

    typedef server_t::connection_t connection_t;
    typedef rack::response< connection_t > response_t;

    extern "C" void* call_ruby_application( void* that );

    class bayeux_server : private pubsub::adapter, bayeux::adapter< VALUE >, rack::call_back_interface
    {
    public:
        bayeux_server( VALUE application, VALUE configuration );

        void run();

    private:
        boost::shared_ptr< server::async_response > on_bayeux_request(
                    const boost::shared_ptr< connection_t >&                connection,
                    const boost::shared_ptr< const http::request_header >&  request );

        boost::shared_ptr< server::async_response > on_request(
                    const boost::shared_ptr< connection_t >&                connection,
                    const boost::shared_ptr< const http::request_header >&  request );

        static pubsub::configuration pubsub_config( VALUE configuration );

        void validate_node( const pubsub::node_name& node_name, const boost::shared_ptr< pubsub::validation_call_back>& );
        void authorize( const boost::shared_ptr< pubsub::subscriber >&, const pubsub::node_name& node_name, const boost::shared_ptr< pubsub::authorization_call_back >&);
        void node_init( const pubsub::node_name& node_name, const boost::shared_ptr< pubsub::initialization_call_back >&);

        std::pair< bool, json::string > handshake( const json::value& ext, VALUE& session );
        std::pair< bool, json::string > publish( const json::string& channel, const json::value& data,
            const json::object& message, VALUE& session, pubsub::root& root );

        void run_queue();

        virtual std::vector< char > call( const std::vector< char >& body, const http::request_header& request );

        boost::asio::io_service             queue_;
        const VALUE                         app_;
        pubsub::root                        root_;
        server::secure_session_generator    session_generator_;
        bayeux::connector<>                 connector_;
        rack::call_queue< response_t >      response_queue_;
        server_t                            server_;

    };

    bayeux_server::bayeux_server( VALUE application, VALUE configuration )
        : queue_()
        , app_( application )
        , root_( queue_, *this, pubsub_config( configuration ) )
        , connector_( queue_, root_, session_generator_, *this, bayeux::configuration() )
        , server_( queue_, 0, std::cout )
    {
        server_.add_action( "/bayeux", boost::bind( &bayeux_server::on_bayeux_request, this, _1, _2 ) );
        server_.add_action( "/", boost::bind( &bayeux_server::on_request, this, _1, _2 ) );

        const unsigned port = rack::from_hash( configuration, "Port" );

        using namespace boost::asio::ip;
        server_.add_listener( tcp::endpoint( address( address_v4::any() ), port ) );
        server_.add_listener( tcp::endpoint( address( address_v6::any() ), port ) );
    }

    void bayeux_server::run_queue()
    {
        try
        {
            queue_.run();
        }
        catch ( ... )
        {
            std::cout << "exception in run_queue()" << std::endl;
        }
    }

    void bayeux_server::run()
    {
        boost::thread queue_runner( boost::bind( &bayeux_server::run_queue, this ) );

        response_queue_.process_request( *this );

        queue_.stop();
        queue_runner.join();
    }


    boost::shared_ptr< server::async_response > bayeux_server::on_bayeux_request(
                const boost::shared_ptr< connection_t >&                connection,
                const boost::shared_ptr< const http::request_header >&  request )
    {
        return connector_.create_response( connection, request );
    }

    boost::shared_ptr< server::async_response > bayeux_server::on_request(
                const boost::shared_ptr< connection_t >&                connection,
                const boost::shared_ptr< const http::request_header >&  request )
    {
        const boost::shared_ptr< server::async_response > result(
            new rack::response< connection_t >( connection, request, queue_, response_queue_ ) );

        return result;
    }

    pubsub::configuration bayeux_server::pubsub_config( VALUE /* configuration */ )
    {
        return pubsub::configuration();
    }

    void bayeux_server::validate_node( const pubsub::node_name& node_name, const boost::shared_ptr< pubsub::validation_call_back>& )
    {
    }

    void bayeux_server::authorize( const boost::shared_ptr< pubsub::subscriber >&, const pubsub::node_name& node_name, const boost::shared_ptr< pubsub::authorization_call_back >&)
    {
    }

    void bayeux_server::node_init( const pubsub::node_name& node_name, const boost::shared_ptr< pubsub::initialization_call_back >&)
    {
    }

    std::pair< bool, json::string > bayeux_server::handshake( const json::value& /* ext */, VALUE& session )
    {
        session = Qnil;

        return std::pair< bool, json::string >();
    }

    std::pair< bool, json::string > bayeux_server::publish( const json::string& channel, const json::value& data,
        const json::object& message, VALUE& session, pubsub::root& root )
    {
        return std::pair< bool, json::string >();
    }

    static void fill_header( VALUE environment, const http::request_header& request )
    {
        using namespace rack;

        rb_hash_aset( environment,
            rb_str_new2( "REQUEST_METHOD" ),
            rb_str_new2( tools::as_string( request.method() ).c_str() ) );

        tools::substring scheme, authority, path, query, fragment;
        http::split_url( request.uri(), scheme, authority, path, query, fragment );

        // SCRIPT_NAME + PATH_INFO should yield path, where PATH_INFO is the 'mounting point'
        // we don't have a mounting point, thus SCRIPT_NAME = empty and PATH_INFO = path
        rb_hash_aset( environment, rb_str_new2( "SCRIPT_NAME" ), rb_str_new( "", 0 ) );
        rb_hash_aset( environment, rb_str_new2( "PATH_INFO"), rb_str_new_sub( path ) );
        rb_hash_aset( environment, rb_str_new2( "QUERY_STRING" ), rb_str_new_sub( query ) );

        rb_hash_aset( environment, rb_str_new2( "SERVER_NAME" ), rb_str_new_sub( request.host() ) );
        rb_hash_aset( environment, rb_str_new2( "SERVER_PORT" ), INT2FIX( request.port() ) );
    }

    std::vector< char > bayeux_server::call( const std::vector< char >& body, const http::request_header& request )
    {
        VALUE hash = rb_hash_new();

        // call the application callback
        fill_header( hash, request );
        static const ID func_name = rb_intern("call");

        assert( func_name );
        VALUE ruby_result = rb_funcall( app_, func_name, 1, hash );

        if ( TYPE( ruby_result ) == T_ARRAY )
        {
            const int result_size = RARRAY_LEN( ruby_result );
std::cout << "result ist array: "            << result_size << std::endl;
            if ( result_size  == 0 )
            {
std::cout << "stopping..." << std::endl;
                response_queue_.stop();
            }
            else if ( result_size == 3 )
            {

            }
            else
            {

            }
        }
        // contruct a response header

        // deliver the result
        return std::vector< char >();
    }
}

extern "C" VALUE run_bayeux( int argc, VALUE *argv, VALUE obj )
{
    assert( argc == 2 );

    VALUE result = Qfalse;

    try
    {
        bayeux_server server( *argv, *( argv  +1 ) );

        server.run();
        result = Qtrue;
    }
    catch ( const std::exception& e )
    {
        rb_raise( rb_eRuntimeError, "exception calling Sioux.run(): %s", e.what() );
    }
    catch ( ... )
    {
        rb_raise( rb_eRuntimeError, "unknown exception calling Sioux.run()" );
    }

    return result;
}

extern "C" void Init_bayeux()
{
    VALUE class_ = rb_define_class( "SiouxRubyImplementation", rb_cObject );
    rb_define_singleton_method( class_, "run", RUBY_METHOD_FUNC( run_bayeux ), -1 );
}

