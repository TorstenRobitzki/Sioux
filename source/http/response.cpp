// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "http/response.h"
#include "http/parser.h"
#include "tools/split.h"

namespace http {

    response_header::response_header()
    {
    }

    response_header::response_header(const response_header& old_header, std::size_t& remaining, copy_trailing_buffer_t)
        : message_base<response_header>(old_header, remaining, copy_trailing_buffer)
    {
    }

    response_header::response_header(const char* p)
        : message_base<response_header>(p)
    {
    }

    http::http_error_code response_header::code() const
    {
        assert(state() == ok);
        return code_;
    }

    tools::substring response_header::phrase() const
    {
        assert(state() == ok);
        return phrase_;
    }

    bool response_header::start_line_found(const char* start, const char* end)
    {
        assert(start != end);

        tools::substring    htt_version;
        tools::substring    rest, status;

        if ( !tools::split(start, end, ' ', htt_version, rest) ) 
            return false; 

        // phrase is optional
        if ( !tools::split(rest, ' ', status, phrase_) )
        {
            status = rest;
            status.trim(' ');
        }

        if ( !parse_version(htt_version) )
            return false;

        unsigned code;
        if ( !parse_number(status.begin(), status.end(), code) )
            return false;

        code_ = static_cast<http_error_code>(code);

        return code_ >= 100 && code_ < 600;
    }

    message::error_code response_header::end_of_request()
    {
        return ok;
    }

} // namespace http


