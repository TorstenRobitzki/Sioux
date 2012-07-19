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

    // calling rb_intern, results in compiler message
    static ID work_around_gcc_bug( const char* s )
    {
        return rb_intern( s );
    }

    static const ID validate_node_func_name = work_around_gcc_bug( "validate_node" );

    static void validate_node_impl( VALUE adapter, const pubsub::node_name& node_name,
        const boost::shared_ptr< pubsub::validation_call_back >& cb )
    {
        const VALUE hash = node_to_hash( node_name );

        const VALUE result = rb_funcall( adapter, validate_node_func_name, 1, hash );


        RTEST( result ) ? cb->is_valid() : cb->not_valid();
    }

    void adapter::validate_node( const pubsub::node_name& node_name, const boost::shared_ptr< pubsub::validation_call_back >& cb )
    {
        if ( adapter_ == Qnil || !rb_respond_to( adapter_, validate_node_func_name ) )
            return;

        ruby_land_.push( boost::bind( validate_node_impl, adapter_, node_name, cb ) );
    }

    static const ID authorize_func_name = work_around_gcc_bug( "authorize" );

    static void authorize_impl( VALUE adapter, const boost::shared_ptr< pubsub::subscriber >&, const pubsub::node_name& node_name,
        const boost::shared_ptr< pubsub::authorization_call_back >& cb )
    {
        const VALUE hash = node_to_hash( node_name );

        const VALUE result = rb_funcall( adapter, authorize_func_name, 2, Qnil, hash );

        RTEST( result ) ? cb->is_authorized() : cb->not_authorized();
    }

    void adapter::authorize(const boost::shared_ptr< pubsub::subscriber >& user, const pubsub::node_name& node_name,
        const boost::shared_ptr< pubsub::authorization_call_back >& cb )
    {
        if ( adapter_ == Qnil || !rb_respond_to( adapter_, authorize_func_name ) )
            return;

        ruby_land_.push( boost::bind( authorize_impl, adapter_, user, node_name, cb ) );
    }

    static const ID node_init_func_name = work_around_gcc_bug( "node_init" );

    static void node_init_impl( VALUE adapter, const pubsub::node_name& node_name, const boost::shared_ptr< pubsub::initialization_call_back >& cb )
    {
        const VALUE hash = node_to_hash( node_name );

        const VALUE result    = rb_funcall( adapter, node_init_func_name, 1, hash );

        cb->initial_value( ruby_to_json( result, node_name ) );
    }

    void adapter::node_init( const pubsub::node_name& node_name, const boost::shared_ptr< pubsub::initialization_call_back >& cb )
    {
        if ( adapter_ == Qnil || !rb_respond_to( adapter_, node_init_func_name ) )
            return;

        ruby_land_.push( boost::bind( node_init_impl, adapter_, node_name, cb ) );
    }

}
