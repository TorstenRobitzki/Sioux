// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SOURCE_SERVER_REQUEST_H
#define SOURCE_SERVER_REQUEST_H

#include "http/http.h"
#include "tools/substring.h"
#include <boost/asio/buffer.hpp>
#include <iosfwd>

namespace server
{

    struct header
    {
        tools::substring    name;
        tools::substring    value;
    };

    /**
     * @brief access and parsing of http request headers
     */
	class request_header
	{
    public:
        request_header();

        enum copy_trailing_buffer_t {copy_trailing_buffer};

        /**
         * @brief constructs a new request_header with the remaining data past the
         * last read request header
         *
         * @attention after constructing a request header this way, it might be possible
         * that this header is too already complete.
         *
         * @param remaining returns the unparsed bytes. If not 0, parse() can be called with this
         * information.
         */
        request_header(const request_header& old_header, std::size_t& remaining, copy_trailing_buffer_t);

        /**
         * @brief returns the write pointer and remaining buffer size 
         * 
         * It guarantied that pointer is not null and the size is not 0
         */
        std::pair<char*, std::size_t> read_buffer();

        /**
         * @brief consumes size byte from the read_buffer()
         *
         * The function returns true, if parsing the request header is done, ether by
         * success, or by any error.
         *
         * @pre state() have to return parsing
         */
        bool parse(std::size_t size);

        enum error_code
        {
            /** request is parsed and valid */
            ok,
            /** request couldn't be parsed, because an internal buffer is full */
            buffer_full,
            /** the request contains syntactical errors */
            syntax_error,
            /** parsing isn't finished jet */
            parsing
        };

        error_code state() const;

        /**
         * @brief returns true, when there is no reason to read further requests from the connection.
         */
        bool continue_reading() const;

        /*
         * getters for the header informations.
         * @pre a prior call to parse() returned true
         * @pre state() returns ok
         */
        unsigned                major_version() const;
        unsigned                minor_version() const;

        http::http_method_code  method() const;

        tools::substring        uri() const;
    private:
        void crlf_found(const char* start, const char* end);
        void request_line_found(const char* start, const char* end);
        void header_found(const char* start, const char* end);

        void end_of_request();

        void parse_error();

        char                    buffer_[1024];
        std::size_t             write_ptr_;
        std::size_t             parse_ptr_; // already consumed including trailing CRLF
        std::size_t             read_ptr_;  // read, but no CRLF found so far
        error_code              error_;

        enum {
            expect_request_line,
            expect_header,
        } parser_state_;

        unsigned                major_version_;
        unsigned                minor_version_;

        http::http_method_code  method_;

        tools::substring        uri_;

        std::vector<header>     headers_;
	};

    std::ostream& operator<<(std::ostream& out, request_header::error_code e);

} // namespace server

#endif 

