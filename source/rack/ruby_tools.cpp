// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "rack/ruby_tools.h"
#include "rack/log.h"
#include "pubsub/node.h"
#include "json/json.h"
#include "tools/exception_handler.h"
#include <sstream>

namespace rack
{
    static VALUE access_hash( VALUE hash, const char* entry )
    {
        static const ID func_name = rb_intern("[]");
        assert( func_name );

        VALUE ruby_result = rb_funcall( hash, func_name, 1, rb_str_new2( entry ) );

        if ( ruby_result == Qnil )
            rb_raise( rb_eArgError, "no entry named: %s found", entry );

        return ruby_result;
    }

    int from_hash( VALUE hash, const char* entry )
    {
        VALUE ruby_result = access_hash( hash, entry );

        if ( TYPE( ruby_result ) != T_FIXNUM )
            rb_raise( rb_eTypeError, "expected Fixnum for %s", entry );

        return FIX2INT( ruby_result );
    }

    bool bool_from_hash( VALUE hash, const char* entry )
    {
        VALUE ruby_result = access_hash( hash, entry );

        if ( TYPE( ruby_result ) != T_TRUE  && TYPE( ruby_result ) != T_FALSE )
            rb_raise( rb_eTypeError, "expected boolean for %s", entry );

        return TYPE( ruby_result ) == T_TRUE;
    }

    VALUE rb_str_new_sub( const tools::substring& s )
    {
        return rb_str_new( s.begin(), s.size() );
    }


    VALUE rb_str_new_std( const std::string& s )
    {
        return rb_str_new( s.data(), s.size() );
    }

    tools::substring rb_str_to_sub( VALUE str )
    {
        Check_Type( str, T_STRING );
        return tools::substring( RSTRING_PTR( str ), RSTRING_PTR( str ) + RSTRING_LEN( str ) );
    }

    std::string rb_str_to_std( VALUE str )
    {
        Check_Type( str, T_STRING );
        return std::string( RSTRING_PTR( str ), RSTRING_PTR( str ) + RSTRING_LEN( str ) );
    }

    json::string rb_str_to_json( VALUE str )
    {
        Check_Type( str, T_STRING );
        return json::string( RSTRING_PTR( str ), RSTRING_PTR( str ) + RSTRING_LEN( str ) );

    }
    VALUE node_to_hash( const pubsub::node_name& node_name )
    {
        VALUE hash = rb_hash_new();

        const pubsub::node_name::key_list& keys = node_name.keys();
        for ( pubsub::node_name::key_list::const_iterator key = keys.begin(); key != keys.end(); ++key )
        {
            VALUE key_name = rb_str_new_std( key->domain().name() );
            VALUE value    = rb_str_new_std( key->value() );

            rb_hash_aset( hash, key_name, value );
        }

        return hash;
    }

    extern "C" int each_subriber_hash_value( VALUE key, VALUE value, VALUE node_ptr )
    {
        pubsub::node_name& node = *reinterpret_cast< pubsub::node_name* >( node_ptr );

        std::string k = rack::rb_str_to_std( StringValue( key ) );
        std::string v = rack::rb_str_to_std( StringValue( value ) );

        node.add( pubsub::key( pubsub::key_domain( k ), v ) );
        return ST_CONTINUE;
    }

    pubsub::node_name hash_to_node( VALUE ruby_node )
    {
        VALUE hash = rb_check_convert_type( ruby_node, T_HASH, "Hash", "to_hash" );

        pubsub::node_name node;
        rb_hash_foreach( hash, reinterpret_cast< int (*)(...) >( each_subriber_hash_value ), reinterpret_cast< VALUE >( &node ) );

        return node;
    }

    VALUE json_to_ruby( const json::value& data )
    {
        static const ID    parse_function = rb_intern( "parse" );
        static const ID    json_obj_name  = rb_intern( "JSON" );

        const VALUE json_parser    = rb_const_get( rb_cObject, json_obj_name );
        assert( !NIL_P( json_parser ) );

        // JSON.parse( "#{data}" )[ 0 ] hack, as a JSON expression must be an object or array on the top level according
        // to rfc4627
        return *RARRAY_PTR( rb_funcall( json_parser, parse_function, 1, rb_str_new_std( "[" + data.to_json() + "]" ) ) );
    }

    json::value ruby_to_json( VALUE ruby_json, const pubsub::node_name& node_name )
    {
        // default, if anything goes wrong is null
        json::value result = json::null();

        const ID to_json_func = rb_intern( "to_json" );

        if ( !rb_respond_to( ruby_json, to_json_func ) )
        {
            LOG_WARNING( log_context << "initial value for node: \"" << node_name << "\" does not respond to \"to_json\"." );
            return result;
        }

        VALUE ruby_string = rb_funcall( ruby_json, to_json_func, 0 );
        Check_Type( ruby_string, T_STRING );

        try
        {
            result = json::parse( RSTRING_PTR( ruby_string ), RSTRING_PTR( ruby_string ) + RSTRING_LEN( ruby_string ) );
        }
        catch ( ... )
        {
            LOG_ERROR( log_context << "while trying to convert value for node: \"" << node_name << "\" => " << tools::exception_text() );
            LOG_ERROR( log_context << "expression was: \"" << rb_str_to_sub( ruby_string ) << "\"" );
        }

        return result;
    }

}
