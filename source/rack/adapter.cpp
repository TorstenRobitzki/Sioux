// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "rack/adapter.h"
#include "rack/ruby_tools.h"
#include "rack/ruby_land_queue.h"
#include "rack/log.h"
#include "json/json.h"
#include "tools/exception_handler.h"
#include "pubsub/node.h"
#include <boost/bind.hpp>

#include <ruby.h>

namespace rack
{
    adapter::adapter( VALUE ruby_adapter, ruby_land_queue& ruby_land )
        : adapter_( ruby_adapter )
        , ruby_land_( ruby_land )
    {
    }

    adapter::pubsub_publish_result_t convert_call_back_result( VALUE answer, const char* error_context_msg )
    {
        if ( TYPE( answer ) != T_ARRAY )
        {
            LOG_ERROR( log_context << error_context_msg << " result is not a ruby array" );
        }
        else if ( RARRAY_LEN( answer ) != 2 )
        {
            LOG_ERROR( log_context << error_context_msg << " size of returned array is not 2" );
        }
        else
        {
            const VALUE first_arg  = RARRAY_PTR( answer )[ 0 ];
            const VALUE second_arg = RARRAY_PTR( answer )[ 1 ];

            return adapter::pubsub_publish_result_t(
                json::array( ruby_to_json( first_arg ) ),
                static_cast< http::http_error_code >( NUM2INT( second_arg ) ) );
        }

        return adapter::pubsub_publish_result_t(
            json::array( json::string( "internal error" ) ), http::http_internal_server_error );
    }

    adapter::pubsub_publish_result_t adapter::publish( const json::value& body, VALUE root )
    {
        static const ID publish_function = rb_intern( "publish" );
        static const char* error_context_msg = "while trying to upcall pubsub publish handler: \"";

        try
        {
            if ( !rb_respond_to( adapter_, publish_function ) )
            {
                return pubsub_publish_result_t(
                    json::array( json::string( "adapter does not respond to publish." ) ),
                    http::http_internal_server_error );
            }
            else
            {
                const json::value undecorated_body = body.upcast< json::array >().at( 0 );
                const VALUE ruby_value = rack::json_to_ruby( undecorated_body );

                return convert_call_back_result(
                    rb_funcall( adapter_, publish_function, 2, ruby_value, root ),
                    error_context_msg );
            }
        }
        catch ( ... )
        {
            LOG_ERROR( log_context << error_context_msg << body << "\" => "
                << tools::exception_text() );

            // attention!, the error text is communicated to the outside.
            return pubsub_publish_result_t(
                json::array( json::string( "internal error" ) ),
                http::http_internal_server_error );
        }

    }

    static void does_not_repond_to( const char* func )
    {
        LOG_WARNING( log_context << "user adapter does not respond_to \"" << func << "\"" );
    }

    static void validate_node_impl( VALUE adapter, const pubsub::node_name& node_name,
        const boost::shared_ptr< pubsub::validation_call_back >& cb )
    {
        static const char * const func_name = "validate_node";
        static const ID validate_node_func_name = rb_intern( func_name );

        VALUE result = Qnil;

        if ( rb_respond_to( adapter, validate_node_func_name ) )
        {
            const VALUE hash = node_to_hash( node_name );
            result = rb_funcall( adapter, validate_node_func_name, 1, hash );
        }
        else
        {
            does_not_repond_to( func_name );
        }

        RTEST( result ) ? cb->is_valid() : cb->not_valid();
    }

    void adapter::validate_node( const pubsub::node_name& node_name, const boost::shared_ptr< pubsub::validation_call_back >& cb )
    {
        if ( adapter_ != Qnil )
            ruby_land_.push( boost::bind( validate_node_impl, adapter_, node_name, cb ) );
    }


    static void authorize_impl( VALUE adapter, const boost::shared_ptr< pubsub::subscriber >&, const pubsub::node_name& node_name,
        const boost::shared_ptr< pubsub::authorization_call_back >& cb )
    {
        static const char * const func_name = "authorize";
        static const ID authorize_func_name = rb_intern( func_name );

        VALUE result = Qnil;

        if ( rb_respond_to( adapter, authorize_func_name ) )
        {
            const VALUE hash = node_to_hash( node_name );
            result = rb_funcall( adapter, authorize_func_name, 2, Qnil, hash );
        }
        else
        {
            does_not_repond_to( func_name );
        }

        RTEST( result ) ? cb->is_authorized() : cb->not_authorized();
    }

    void adapter::authorize(const boost::shared_ptr< pubsub::subscriber >& user, const pubsub::node_name& node_name,
        const boost::shared_ptr< pubsub::authorization_call_back >& cb )
    {
        if ( adapter_ != Qnil )
            ruby_land_.push( boost::bind( authorize_impl, adapter_, user, node_name, cb ) );
    }


    static void node_init_impl( VALUE adapter, const pubsub::node_name& node_name, const boost::shared_ptr< pubsub::initialization_call_back >& cb )
    {
        static const char * const func_name = "node_init";
        static const ID node_init_func_name = rb_intern( "node_init" );

        VALUE result = Qnil;

        if ( rb_respond_to( adapter, node_init_func_name ) )
        {
            const VALUE hash = node_to_hash( node_name );
            result    = rb_funcall( adapter, node_init_func_name, 1, hash );
        }
        else
        {
            does_not_repond_to( func_name );
        }

        const json::value json_result = ruby_to_json( result, node_name );
        LOG_WARNING( log_context << node_name << " initialized to " <<  json_result );

        cb->initial_value( json_result );
    }

    void adapter::node_init( const pubsub::node_name& node_name, const boost::shared_ptr< pubsub::initialization_call_back >& cb )
    {
        if ( adapter_ != Qnil )
            ruby_land_.push( boost::bind( node_init_impl, adapter_, node_name, cb ) );
    }

    void adapter::invalid_node_subscription(const pubsub::node_name& node, const boost::shared_ptr<pubsub::subscriber>&)
    {
        LOG_WARNING( log_context << node << " is invalid!" );
    }

    void adapter::unauthorized_subscription(const pubsub::node_name& node, const boost::shared_ptr<pubsub::subscriber>&)
    {
        LOG_WARNING( log_context << "unauthorized subscription to " << node << " was rejected." );
    }

    void adapter::initialization_failed(const pubsub::node_name& node)
    {
        LOG_WARNING( log_context << "failed to initialize " << node );
    }

}
