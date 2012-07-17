// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "rack/ruby_tools.h"

namespace rack
{
    int from_hash( VALUE hash, const char* entry )
    {
        static const ID func_name = rb_intern("[]");
        assert( func_name );

        VALUE ruby_result = rb_funcall( hash, func_name, 1, rb_str_new2( entry ) );

        if ( !ruby_result )
            rb_raise( rb_eArgError, "no entry named: %s found", entry );

        if ( TYPE( ruby_result ) != T_FIXNUM )
            rb_raise( rb_eTypeError, "expected Fixnum for %s", entry );

        return FIX2INT( ruby_result );
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

}
