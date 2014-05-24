#include "rack/call_rack_application.h"
#include "rack/ruby_tools.h"
#include "rack/log.h"
#include "rack/ruby_land_queue.h"
#include "http/filter.h"
#include "http/request.h"
#include "http/parser.h"
#include "http/server_header.h"
#include "tools/split.h"
#include "tools/asstring.h"

static void fill_http_headers( VALUE environment, const http::request_header& request )
{
    static const http::filter headers_without_prefix( "Content-Length, Content-Type" );
    static const ID upcase = rb_intern( "upcase!" );

    for ( http::request_header::const_iterator header = request.begin(), end = request.end(); header != end; ++header )
    {
        const tools::substring name( header->name() );
        std::string header_name( name.begin(), name.end() );
        std::replace( header_name.begin(), header_name.end(), '-', '_' );

        VALUE header_name_cgi = headers_without_prefix( header->name() )
            ? rack::rb_str_new_std( header_name )
            : rb_str_concat( rb_str_new2( "HTTP_" ), rack::rb_str_new_std( header_name ) );

        rb_funcall( header_name_cgi, upcase, 0 );

        rb_hash_aset( environment, header_name_cgi, rack::rb_str_new_sub( header->value() ) );
    }
}

static void fill_header( VALUE environment, const http::request_header& request )
{
    using namespace rack;

    rb_hash_aset( environment, rb_str_new2( "REQUEST_METHOD" ), rb_str_new2( tools::as_string( request.method() ).c_str() ) );

    tools::substring scheme, authority, path, query, fragment;
    http::split_url( request.uri(), scheme, authority, path, query, fragment );

    // SCRIPT_NAME + PATH_INFO should yield path, where PATH_INFO is the 'mounting point'
    // we don't have a mounting point, thus SCRIPT_NAME = empty and PATH_INFO = path
    rb_hash_aset( environment, rb_str_new2( "SCRIPT_NAME" ), rb_str_new( "", 0 ) );
    rb_hash_aset( environment, rb_str_new2( "PATH_INFO"), rb_str_new_sub( path ) );
    rb_hash_aset( environment, rb_str_new2( "QUERY_STRING" ), rb_str_new_sub( query ) );

    rb_hash_aset( environment, rb_str_new2( "SERVER_NAME" ), rb_str_new_sub( request.host() ) );
    rb_hash_aset( environment, rb_str_new2( "SERVER_PORT" ), INT2FIX( request.port() ) );

    rb_hash_aset( environment, rb_str_new2( "rack.url_scheme" ), rb_str_new2( "http" ) );
    rb_hash_aset( environment, rb_str_new2( "rack.multithread" ), Qfalse );
    rb_hash_aset( environment, rb_str_new2( "rack.multiprocess" ), Qfalse );
    rb_hash_aset( environment, rb_str_new2( "rack.run_once" ), Qfalse );

    fill_http_headers( environment, request );
}

static void fill_endpoint( VALUE environment, const boost::asio::ip::tcp::endpoint& endpoint )
{
    rb_hash_aset( environment, rb_str_new2( "REMOTE_ADDR" ), rack::rb_str_new_std( endpoint.address().to_string() ) );
    rb_hash_aset( environment, rb_str_new2( "REMOTE_PORT" ), rack::rb_str_new_std( tools::as_string( endpoint.port() ) ) );
}

extern "C"
{
    static VALUE call_ruby_cb( VALUE* params )
    {
        static const ID func_name = rb_intern("call");

        assert( TYPE( params[ 1 ] ) == T_HASH );
        return rb_funcall( params[ 0 ], func_name, 1, params[ 1 ] );
    }

    static VALUE rescue_ruby( VALUE /* arg */, VALUE exception )
    {
        VALUE error_msg = rb_str_new2( "error calling application: " );
        error_msg = rb_str_concat( error_msg, rb_funcall( exception, rb_intern( "message" ), 0 ) );
        error_msg = rb_str_concat( error_msg, rb_str_new2( "\n" ) );

        VALUE back_trace = rb_funcall( exception, rb_intern( "backtrace" ), 0 );
        back_trace = rb_funcall( back_trace, rb_intern( "join" ), 1, rb_str_new2( "\n" ) );

        error_msg = rb_str_concat( error_msg, back_trace );

        return error_msg;
    }
}

std::vector< char > rack::call_rack_application( const std::vector< char >& body, const http::request_header& request,
    const boost::asio::ip::tcp::endpoint& endpoint, VALUE application, ruby_land_queue& queue )
{
    VALUE hash = rb_hash_new();

    fill_header( hash, request );
    fill_endpoint( hash, endpoint );
    rb_hash_aset( hash, rb_str_new2( "rack.input" ), rb_str_new( &body[ 0 ], body.size() ) );

    VALUE func_args[ 2 ] = { application, hash };

    // call the application callback
    VALUE ruby_result = rb_rescue2(
        reinterpret_cast< VALUE (*)( ANYARGS ) >( &call_ruby_cb ), reinterpret_cast< VALUE >( &func_args[ 0 ] ),
        reinterpret_cast< VALUE (*)( ANYARGS ) >( &rescue_ruby ), Qnil,
        rb_eException, static_cast< VALUE >( 0 ) );

    if ( TYPE( ruby_result ) == T_STRING )
    {
        LOG_ERROR( log_context << rack::rb_str_to_sub( ruby_result ) );
        return std::vector< char >();
    }

    assert ( TYPE( ruby_result ) == T_ARRAY );
    const int result_size = RARRAY_LEN( ruby_result );

    if ( result_size  == 0 )
    {
        queue.stop();
        return std::vector< char >();
    }

    assert( result_size == 4 );
    VALUE ruby_error   = rb_ary_pop( ruby_result );
    VALUE ruby_body    = rb_ary_pop( ruby_result );
    VALUE ruby_headers = rb_ary_pop( ruby_result );
    VALUE ruby_status  = rb_ary_pop( ruby_result );

    assert( TYPE( ruby_error ) == T_STRING );
    assert( TYPE( ruby_body ) == T_STRING );
    assert( TYPE( ruby_headers ) == T_STRING );
    assert( TYPE( ruby_status ) == T_FIXNUM );

    const std::string status_line = http::status_line( "1.1", static_cast< http::http_error_code >( NUM2INT( ruby_status ) ) );

    VALUE result = rb_str_new( status_line.data(), status_line.size() );
    result = rb_str_plus( result, rb_str_new2( SIOUX_SERVER_HEADER ) );
    result = rb_str_plus( result, ruby_headers );
    result = rb_str_plus( result, ruby_body );

    if ( RSTRING_LEN( ruby_error ) )
        LOG_ERROR( log_context << rack::rb_str_to_sub( ruby_error ) );

    return std::vector< char >( RSTRING_PTR( result ), RSTRING_PTR( result ) + RSTRING_LEN( result ) );
}
