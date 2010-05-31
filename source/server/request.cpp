// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/request.h"
#include "http/http.h"
#include "tools/iterators.h"
#include "tools/split.h"
#include "http/parser.h"
#include <algorithm>
#include <cassert>
#include <cstring>

namespace server {


    //////////////////////////
    // class request_header
    request_header::request_header()
        : write_ptr_(0)
        , parse_ptr_(0)
        , read_ptr_(0)
        , error_(parsing)
        , parser_state_(expect_request_line)
    {
    }

    request_header::request_header(const request_header& old_header, std::size_t& remaining, copy_trailing_buffer_t)
        : write_ptr_(0)
        , parse_ptr_(0)
        , read_ptr_(0)
        , error_(parsing)
        , parser_state_(expect_request_line)
    {
        std::copy(&old_header.buffer_[old_header.parse_ptr_], &old_header.buffer_[old_header.write_ptr_], &buffer_[0]);
        remaining = old_header.write_ptr_ - old_header.parse_ptr_;

        // the last request_header must have signaled a buffer-full error
        assert(remaining != sizeof buffer_);
    }
    
    request_header::request_header(const char* source)
        : write_ptr_(0)
        , parse_ptr_(0)
        , read_ptr_(0)
        , error_(parsing)
        , parser_state_(expect_request_line)
    {
        const std::size_t max = std::min(sizeof buffer_, std::strlen(source));
        std::strncpy(buffer_, source, max);

        parse(max);
    }

    std::pair<char*, std::size_t> request_header::read_buffer()
    {
        assert(write_ptr_ <= sizeof buffer_);
        return std::make_pair(&buffer_[write_ptr_], sizeof buffer_ - write_ptr_);
    }
    
    bool request_header::parse(std::size_t size)
    {
        assert(size);
        assert(error_ == parsing);
        write_ptr_ += size;

        assert(write_ptr_ <= sizeof buffer_);

        for ( std::size_t i = read_ptr_; error_ == parsing && read_ptr_ != write_ptr_; )
        {
            assert(read_ptr_ < write_ptr_);
            assert(parse_ptr_ < write_ptr_);
            assert(parse_ptr_ <= read_ptr_);

            // seek for CR         
            for ( ; i != write_ptr_-1 && buffer_[i] != '\r'; ++i )
                ;

            // no \r found, restart seeking at the currently last buffer position
            if ( i == write_ptr_-1 )
            {
                read_ptr_  = i;

                if ( write_ptr_ == sizeof buffer_ )
                    error_ = buffer_full;

                return error_ != parsing;
            }

            assert(i < write_ptr_-1);

            if ( buffer_[i+1] == '\n' )
            {
                crlf_found(&buffer_[parse_ptr_], &buffer_[i]);

                i += 2;
                parse_ptr_ = read_ptr_ = i;
            } else
            {
                i += 2;
                read_ptr_ = i;
            }
        }

        if ( write_ptr_ == sizeof buffer_ && error_ == parsing )
            error_ = buffer_full;

        return error_ != parsing;
    }
    
    void request_header::crlf_found(const char* start, const char* end)
    {
        if ( parser_state_ == expect_request_line )
        {
            // ignore empty lines at the start 
            if ( start == end )
                return;

            request_line_found(start, end);
        }
        else if ( parser_state_ == expect_header )
        {
            if ( start == end )
            {
               end_of_request();
            }
            else
            {
                header_found(start, end);
            }
        }
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

    void request_header::request_line_found(const char* start, const char* end)
    {
        assert(start != end);

        tools::substring    method_text;
        tools::substring    rest;

        if ( !tools::split(start, end, ' ', method_text, rest) )
            return parse_error(); 

        // simple, linear search
        const header_desc* header_entry = tools::begin(valid_headers);
        for ( ; header_entry != tools::end(valid_headers) && method_text != header_entry->name; ++header_entry )
            ;

        if ( header_entry == tools::end(valid_headers) )
            return parse_error(); 

        method_ = header_entry->code;

        tools::substring    version;
        tools::substring    http;
        tools::substring    minor, major;
        if ( !tools::split(rest, ' ', uri_, version) 
          || !tools::split(version, '/', http, version) 
          || !tools::split(version, '.', major, minor) )
        {
            return parse_error();
        }

        if ( !http::parse_version_number(major.begin(), major.end(), major_version_) 
          || !http::parse_version_number(minor.begin(), minor.end(), minor_version_) )
        {
            return parse_error();
        }

        parser_state_ = expect_header;
    }

    void request_header::header_found(const char* start, const char* end)
    {
        assert(start != end);

        header h;
        if ( tools::split(start, end, ':', h.name_, h.value_) )
        {
            h.name_.trim(' ').trim('\t');
            h.value_.trim(' ').trim('\t');

            headers_.push_back(h);
        }
        else
        {
            error_ = syntax_error;
        }
    }

    void request_header::end_of_request() 
    {
        assert(parser_state_ == expect_header);

        // TODO Check, that all required headers are present
        error_ = ok;
    }

    void request_header::parse_error()
    {
        error_ = syntax_error;
    }

    request_header::error_code request_header::state() const
    {
        return error_;
    }
    
    unsigned request_header::major_version() const
    {
        assert(error_ == ok);
        return major_version_;
    }

    unsigned request_header::minor_version() const
    {
        assert(error_ == ok);
        return minor_version_;
    }

    unsigned request_header::milli_version() const
    {
        assert(error_ == ok);
        return 1000 * major_version_ + minor_version_;
    }

    http::http_method_code request_header::method() const
    {
        assert(error_ == ok);
        return method_;
    }

    tools::substring request_header::uri() const
    {
        assert(error_ == ok);
        return uri_;
    }

    tools::substring request_header::text() const
    {
        return tools::substring(&buffer_[0], &buffer_[parse_ptr_]);
    }

    bool request_header::option_available(const char* header_name, const char* option) const
    {
        assert(error_ == ok);
        const header * const h = find_header(header_name);

        if ( h == 0 )
            return false;

        tools::substring    field, rest(h->value());

        while ( tools::split(rest, ',', field, rest) )
        {
            field.trim(' ').trim('\t');
            if ( http::strcasecmp(field.begin(), field.end(), option) == 0 )
                return true;
        }

        rest.trim(' ').trim('\t');
        return http::strcasecmp(rest.begin(), rest.end(), option) == 0;
    }

    const request_header::header* request_header::find_header(const char* header_name) const
    {
        assert(error_ == ok);
        std::vector<header>::const_iterator h = headers_.begin();
        for ( ; h != headers_.end() && http::strcasecmp(h->name_.begin(), h->name_.end(), header_name) != 0; ++h )
            ;

        return h != headers_.end() ? &*h : 0;
    }

    bool request_header::close_after_response() const
    {
        return error_ != ok || milli_version() < 1001 || option_available("connection", "close");
    }

    std::ostream& operator<<(std::ostream& out, request_header::error_code e)
    {
        switch (e)
        {
        case request_header::ok:
            return out << "ok";
        case request_header::buffer_full:
            return out << "buffer_full";
        case request_header::syntax_error:
            return out << "syntax_error";
        case request_header::parsing:
            return out << "parsing";
        default:
            return out << "unknown request_header::error_code: " << static_cast<int>(e);
        }
    }


} // namespace server

