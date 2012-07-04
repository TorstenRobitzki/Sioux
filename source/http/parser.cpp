// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "http/parser.h"
#include "tools/asstring.h"
#include "tools/hexdump.h"
#include "tools/iterators.h"
#include "tools/split.h"

#include <boost/regex.hpp>
#include <sstream>

namespace http {

std::string to_lower(const std::string& s)
{
	std::string result(s);
	to_lower(result);
	
	return result;
}

void to_lower(std::string& s)
{
	to_lower(s.begin(), s.end());
}


namespace {

    template < class String >
    void split_url_impl(const String& url, String& scheme, String& authority, String& path, String& query, String& fragment)
    {
        // according to rfc 3986
        static const boost::regex expression("^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?");

        boost::match_results< typename String::const_iterator > what;

        if ( boost::regex_match( url.begin(), url.end(), what, expression) )
        {
            scheme      = String( what[2].first, what[2].second );
            authority   = String( what[4].first, what[4].second );
            path        = String( what[5].first, what[5].second );
            query       = String( what[7].first, what[7].second );
            fragment    = String( what[9].first, what[9].second );
        }
        else
        {
            throw bad_url( std::string( url.begin(), url.end() ) );
        }
    }
}

void split_url(const std::string& url, std::string& scheme, std::string& authority, std::string& path, std::string& query, std::string& fragment)
{
    split_url_impl( url, scheme, authority, path, query, fragment );
}

void split_url(const tools::substring& url, tools::substring& scheme, tools::substring& authority,
    tools::substring& path, tools::substring& query, tools::substring& fragment)
{
    split_url_impl( url, scheme, authority, path, query, fragment );
}

bad_query::bad_query( const std::string& s ) : std::runtime_error( s )
{
}

static void add_name_value( std::vector< std::pair< tools::substring, tools::substring > >& result, const tools::substring& name_value )
{
    tools::substring name, value;
    if ( tools::split_to_empty( name_value, '=', name, value ) )
    {
        result.push_back( std::make_pair( name, value ) );
    }
    else
    {
        throw bad_query( "bad-query: " + std::string( name_value.begin(), name_value.end() ) );
    }
}

std::vector< std::pair< tools::substring, tools::substring > > split_query( const tools::substring& query )
{
    std::vector< std::pair< tools::substring, tools::substring > > result;

    tools::substring second = query;
    for ( tools::substring first, rest = query; tools::split_to_empty( rest, '&', first, second );  rest = second )
    {
        add_name_value( result, first );
    }

    if ( !second.empty() )
        add_name_value( result, second );

    return result;
}


namespace {
	template < class Iter >
	int read_nibble( Iter begin, Iter end ) {
		if ( begin == end )
			throw bad_url( "Value missing after %" );
		
		if ( *begin >= '0' && *begin <= '9' )
			return static_cast< int >( *begin ) - static_cast< int >( '0' );
		else if ( *begin >= 'a' && *begin <= 'f' )
			return static_cast< int >( *begin ) - static_cast< int >( 'a' ) + 10;
		else if ( *begin >= 'A' && *begin <= 'F' )
			return static_cast< int >( *begin ) - static_cast< int >( 'A' ) + 10;

		throw bad_url( tools::as_string( static_cast< int >( *begin ) ) + " is not a hexdigit." );
	}
}

namespace {
    template < class Iter >
    std::string url_decode( Iter i, Iter end )
    {
        std::string result;

        for ( ; i != end; ++i )
        {
            if ( *i != '%' )
            {
                result.push_back( *i );
            }
            else
            {
                int value = read_nibble( ++i, end ) * 16;
                value += read_nibble( ++i, end );

                result.push_back( static_cast< char >( value ) );
            }
        }

        return result;
    }
}

std::string url_decode(const std::string& s)
{
    return url_decode( s.begin(), s.end() );
}

std::string url_decode( const tools::substring& s )
{
    return url_decode( s.begin(), s.end() );
}

static bool special( char c )
{
    static const char not_encoded_chars[] = "-_.~";
    static const char * const end = tools::end( not_encoded_chars ) - 1;

    return std::find( tools::begin( not_encoded_chars ), end, c ) != end;
}

std::string url_encode( const std::string& s )
{
    std::stringstream out;


    for ( std::string::const_iterator i = s.begin(), end = s.end(); i != end; ++i )
    {
        if ( *i >= 'a' && *i<= 'z' || *i >= 'A' && *i <= 'Z' || *i >= '0' && *i <= '9' || special( *i ) )
        {
            out << *i;
        }
        else
        {
            out << '%';
            tools::print_hex_uppercase( out, *i );
        }
    }

    return out.str();
}

namespace {

    template < class Iter >
    std::string form_decode_impl( Iter begin, Iter end )
    {
        std::string result( begin, end );
        std::replace( result.begin(), result.end(), '+', ' ' );

        return url_decode( result.begin(), result.end() );
    }
}

std::string form_decode( const std::string& s )
{
    return form_decode_impl( s.begin(), s.end() );
}

std::string form_decode( const tools::substring& s )
{
    return form_decode_impl( s.begin(), s.end() );
}

int strcasecmp(const char* begin1, const char* end1, const char* null_terminated_str)
{
    for ( ; begin1 != end1 && *null_terminated_str; ++begin1, ++null_terminated_str)
    {
        const char cl = static_cast<char>(std::tolower(*begin1));
        const char cr = static_cast<char>(std::tolower(*null_terminated_str));

        if ( cl < cr )
            return -1;
        if ( cr < cl )
            return 1;
    }

    if ( *null_terminated_str )
        return -1;

    if ( begin1 != end1 )
        return 1;

    return 0;
}

int strcasecmp(const char* begin1, const char* end1, const char* begin2, const char* end2)
{
    for ( ; begin1 != end1 && begin2 != end2; ++begin1, ++begin2)
    {
        const char cl = static_cast<char>(std::tolower(*begin1));
        const char cr = static_cast<char>(std::tolower(*begin2));

        if ( cl < cr )
            return -1;
        if ( cr < cl )
            return 1;
    }

    if ( begin2 != end2 )
        return -1;

    if ( begin1 != end1 )
        return 1;

    return 0;
}

unsigned xdigit_value(char c)
{
    if ( c >= '0' && c <= '9' )
        return c - '0';

    if ( c >= 'a' && c <= 'f' )
        return c - 'a' +10;

    return c - 'A' +10;
}

////////////////////////////////////////////////
// class bad_url
bad_url::bad_url(const std::string& s)
 : std::runtime_error("Bad-Url: " + s)
{

}


} // namespace sioux

