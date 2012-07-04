// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.


#include <ruby.h>
#include <assert.h>
#include <iostream>
#include <boost/bind.hpp>

#include "server/server.h"
#include "server/secure_session_generator.h"
#include "bayeux/bayeux.h"
#include "bayeux/log.h"
#include "pubsub/pubsub.h"
#include "pubsub/configuration.h"
#include <boost/date_time/posix_time/posix_time.hpp>


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

    typedef typename server_t::connection_t connection_t;

    class bayeux_connection : public server::async_response
    {
    public:
        bayeux_connection(
            const boost::shared_ptr< connection_t >&                connection,
            const boost::shared_ptr< const http::request_header >&  request,
            VALUE application );
    private:
        void start();

        const boost::shared_ptr< connection_t >                 connection_;
        const boost::shared_ptr< const http::request_header >   request_;
        const VALUE                                             app_;
    };

    bayeux_connection::bayeux_connection(
        const boost::shared_ptr< connection_t >&                connection,
        const boost::shared_ptr< const http::request_header >&  request,
        VALUE                                                   application )
        : connection_( connection )
        , request_( request )
        , app_( application )
    {

    }

    void bayeux_connection::start()
    {
        // read a body if exists
        // create a hash to call app.call()
        // call app.call()
        // deliver the result
    }

    class bayeux_server : private pubsub::adapter, bayeux::adapter< VALUE >
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

        boost::asio::io_service             queue_;
        const VALUE                         app_;
        pubsub::root                        root_;
        server::secure_session_generator    session_generator_;
        bayeux::connector<>                 connector_;
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
    }

    void bayeux_server::run()
    {
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
        const boost::shared_ptr< server::async_response > result( new bayeux_connection( connection, request, app_ ) );

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

#define EXPORT __attribute__((visibility("default")))

extern "C" EXPORT void Init_bayeux()
{
    VALUE class_        = rb_define_class( "SiouxRubyImplementation", rb_cObject );
    rb_define_singleton_method( class_, "run", RUBY_METHOD_FUNC( run_bayeux ), -1 );
}

