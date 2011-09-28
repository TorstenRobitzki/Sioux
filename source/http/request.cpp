// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "http/request.h"
#include "http/parser.h"
#include "tools/split.h"
#include "tools/iterators.h"

namespace http {

    /////////////////////////
    // struct request_data
    details::request_data::request_data()
        : method_()
        , uri_()
        , port_(80)
        , host_()
        , error_code_(http_ok)
    {
    }

    /////////////////////////
    // class request_header
    request_header::request_header()
        : details::request_data()
        , message_base<request_header>()
    {
    }

    request_header::request_header(const request_header& old_header, std::size_t& remaining, copy_trailing_buffer_t)
        : message_base< request_header >( old_header, remaining, copy_trailing_buffer )
    {
    }

    request_header::request_header( const boost::asio::const_buffers_1& old_body, std::size_t& remaining )
    	: message_base< request_header >( old_body, remaining )
    {
    }

    request_header::request_header(const char* p) 
        : message_base<request_header>(p)
    {
    }

    namespace {
        struct header_desc
        {
            const http::http_method_code    code;
            const char*                     name;
        };

        const header_desc valid_headers[] = 
        {
            { http::http_options, "OPTIONS" },
            { http::http_get, "GET" },
            { http::http_head, "HEAD" },
            { http::http_post, "POST" },
            { http::http_put, "PUT" }, 
            { http::http_delete, "DELETE" },
            { http::http_trace, "TRACE" },
            { http::http_connect, "CONNECT" }
        };
    }

    bool request_header::start_line_found(const char* start, const char* end)
    {
        assert(start != end);

        tools::substring    method_text;
        tools::substring    rest;

        if ( !tools::split(start, end, ' ', method_text, rest) )
            return false; 

        // simple, linear search
        const header_desc* header_entry = tools::begin(valid_headers);
        for ( ; header_entry != tools::end(valid_headers) && method_text != header_entry->name; ++header_entry )
            ;

        if ( header_entry == tools::end(valid_headers) )
            return false; 

        method_ = header_entry->code;

        tools::substring    version;
        return tools::split(rest, ' ', uri_, version) && parse_version(version);
    }

    message::error_code request_header::end_of_request()
    {
        const header* const host_header = find_header_impl("host");

        if ( host_header == 0 )
            return bad_request();

        const tools::substring host = host_header->value();

        tools::substring::const_iterator pos = host.end();
        for ( ; pos != host.begin() && std::isdigit(*(pos-1)) != 0; --pos )
            ;

        if ( pos != host.begin() && *(pos-1) == ':' )
        {
            if ( pos != host.end() && !parse_number(pos, host.end(), port_) || port_ > 0xffff )
                return bad_request();

            host_ = tools::substring(host.begin(), pos-1);
        }
        else
        {
            host_ = host;
        }

        return ok;
    }

    http::http_method_code request_header::method() const
    {
        assert(state() == ok);
        return method_;
    }

    tools::substring request_header::uri() const
    {
        assert(state() == ok);
        return uri_;
    }

    tools::substring request_header::host() const
    {
        assert(state() == ok);
        return host_;
    }

    unsigned request_header::port() const
    {
        assert(state() == ok);
        return port_;
    }

    http_error_code request_header::error_code() const
    {
        assert(state() == syntax_error);
        return error_code_;
    }

    bool request_header::body_expected() const
    {
        return find_header("Content-Length") != 0 || find_header("Transfer-Encoding");
    }

    message::error_code request_header::bad_request()
    {
        error_code_ = http_bad_request;
        return message::syntax_error;
    }

    // struct request_url_print
    request_url_print::request_url_print( const request_header& req )
    	: request_( req )
    {
    }

    request_url_print request_url( const request_header& req )
    {
    	return request_url_print( req );
    }

    std::ostream& operator<<( std::ostream& out, const request_url_print& r )
    {
    	if ( r.request_.state() == message::ok )
    	{
			return out << r.request_.method() << " " << r.request_.uri()
					<< "/" << r.request_.host() << ":" << r.request_.port();
    	}
    	else
    	{
    		return out << "state: " << r.request_.state();
    	}
    }


} // namespace http

