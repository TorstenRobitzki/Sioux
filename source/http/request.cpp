// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "http/request.h"
#include "http/parser.h"
#include "tools/split.h"
#include "tools/iterators.h"

namespace http {

    request_header::request_header()
        : message_base<request_header>()
    {
    }

    request_header::request_header(const request_header& old_header, std::size_t& remaining, copy_trailing_buffer_t)
        : message_base<request_header>(old_header, remaining, copy_trailing_buffer)
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


} // namespace http

