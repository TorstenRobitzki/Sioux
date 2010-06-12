// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "http/message.h"
#include "http/request.h"
#include "http/response.h"
#include "http/filter.h"
#include "http/parser.h"
#include "tools/split.h"
#include <algorithm>
#include <cassert>
#include <cstring>


namespace http {

    //////////////////////////
    // class request_header
    template <class Type>
    message_base<Type>::message_base()
        : write_ptr_(0)
        , parse_ptr_(0)
        , read_ptr_(0)
        , error_(parsing)
        , parser_state_(expect_request_line)
    {
    }
    
    template <class Type>
    message_base<Type>::message_base(const Type& old_header, std::size_t& remaining, copy_trailing_buffer_t)
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
    
    template <class Type>
    message_base<Type>::message_base(const char* source)
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

    template <class Type>
    std::pair<char*, std::size_t> message_base<Type>::read_buffer()
    {
        assert(write_ptr_ <= sizeof buffer_);
        return std::make_pair(&buffer_[write_ptr_], sizeof buffer_ - write_ptr_);
    }
    
    template <class Type>
    bool message_base<Type>::parse(std::size_t size)
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
    
    template <class Type>
    bool message_base<Type>::parse_version(const tools::substring& version_text)
    {
        tools::substring    http;
        tools::substring    minor, major, version;

        if ( !tools::split(version_text, '/', http, version) || !tools::split(version, '.', major, minor) )
            return false;

        return  http::parse_number(major.begin(), major.end(), major_version_) 
             && http::parse_number(minor.begin(), minor.end(), minor_version_);
    }

    template <class Type>
    void message_base<Type>::crlf_found(const char* start, const char* end)
    {
        if ( parser_state_ == expect_request_line )
        {
            // ignore empty lines at the start 
            if ( start == end )
                return;

            start_line_ = tools::substring(start, end);
            
            if ( !static_cast<Type*>(this)->start_line_found(start, end) )
                return parse_error();

            parser_state_ = expect_header;
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

    template <class Type>
    void message_base<Type>::header_found(const char* start, const char* end)
    {
        assert(start != end);
        header h;

        // continuation?
        if ( (*start == ' ' || *start == '\t') && !headers_.empty() )
        {
            headers_.back().add_value_line(end);
        }
        else if ( h.parse(start, end) )
        {
            headers_.push_back(h);
        }
        else
        {
            parse_error();
        }
    }

    template <class Type>
    void message_base<Type>::end_of_request() 
    {
        assert(parser_state_ == expect_header);

        // TODO Check, that all required headers are present
        error_ = ok;
    }

    template <class Type>
    void message_base<Type>::parse_error()
    {
        error_ = syntax_error;
    }

    template <class Type>
    typename message_base<Type>::error_code message_base<Type>::state() const
    {
        return error_;
    }
    
    template <class Type>
    unsigned message_base<Type>::major_version() const
    {
        assert(error_ == ok);
        return major_version_;
    }

    template <class Type>
    unsigned message_base<Type>::minor_version() const
    {
        assert(error_ == ok);
        return minor_version_;
    }

    template <class Type>
    unsigned message_base<Type>::milli_version() const
    {
        assert(error_ == ok);
        return 1000 * major_version_ + minor_version_;
    }

    template <class Type>
    tools::substring message_base<Type>::text() const
    {
        return tools::substring(&buffer_[0], &buffer_[parse_ptr_]);
    }

    template <class Type>
    bool message_base<Type>::option_available(const char* header_name, const char* option) const
    {
        assert(error_ == ok);
        const header * const h = find_header(header_name);

        if ( h == 0 )
            return false;

        tools::substring    field, rest(h->value());

        while ( tools::split(rest, ',', field, rest) )
        {
            if ( http::strcasecmp(
                    http::eat_spaces_and_CRLS(field.begin(), field.end()), 
                    http::reverse_eat_spaces_and_CRLS(field.begin(), field.end()),
                    option) == 0 )
            {
                return true;
            }
        }

        rest.trim(' ').trim('\t');
        return http::strcasecmp(
            http::eat_spaces_and_CRLS(rest.begin(), rest.end()),
            http::reverse_eat_spaces_and_CRLS(rest.begin(), rest.end()), option) == 0;
    }

    template <class Type>
    const typename message_base<Type>::header* message_base<Type>::find_header(const char* header_name) const
    {
        assert(error_ == ok);
        std::vector<header>::const_iterator h = headers_.begin();
        for ( ; h != headers_.end() && http::strcasecmp(h->name_.begin(), h->name_.end(), header_name) != 0; ++h )
            ;

        return h != headers_.end() ? &*h : 0;
    }

    template <class Type>
    bool message_base<Type>::close_after_response() const
    {
        return error_ != ok || milli_version() < 1001 || option_available("connection", "close");
    }

    template <class Type>
    std::vector<tools::substring> message_base<Type>::filtered_request_text(const http::filter& not_wanted_header) const
    {
        assert(error_ == ok);
        std::vector<tools::substring>   result;

        tools::substring::iterator current_start = start_line_.begin();
        tools::substring::iterator current_end   = start_line_.end();

        // add headers
        for ( header_list_t::const_iterator h = headers_.begin(); h != headers_.end(); ++h )
        {
            if ( not_wanted_header(h->name()) )
            {
                if ( current_start != current_end )
                    result.push_back( tools::substring(current_start, current_end) );

                current_start = h->end();
            }

            current_end = h->end();
        }

        // current_start now points to the end of the last header, if that was filtered out or
        // to the start of that part of the header that has to be included.
        result.push_back(tools::substring(current_start, &buffer_[parse_ptr_]));

        return result;
    }

    // explizit instanziating
    template class message_base<request_header>;
    template class message_base<response_header>;


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

} // namespace http

