// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SRC_HTTP_MESSAGE_H
#define SIOUX_SRC_HTTP_MESSAGE_H

#include "http/http.h"
#include "http/header.h"
#include <iosfwd>

namespace http 
{
    class filter;

    class message
    {
    public:
        typedef http::header header;

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
    protected:
        ~message() {}
    };

    std::ostream& operator<<(std::ostream& out, message::error_code e);

    /**
     * @brief base class for request_header and response_header
     */
    template <class Type>
    class message_base : public message
    {
    public:
        enum copy_trailing_buffer_t {copy_trailing_buffer};

        /**
         * @brief returns the write pointer and remaining buffer size 
         * 
         * It guarantied that pointer is not null and the size is not 0
         */
        std::pair<char*, std::size_t> read_buffer();

        /**
         * @brief part of the buffer, that was filled, but contains data that was received behind the header
         */
        std::pair<char*, std::size_t> unparsed_buffer();

        /**
         * @brief consumes size byte from the read_buffer()
         *
         * The function returns true, if parsing the request header is done, ether by
         * success, or by any error.
         *
         * @pre state() have to return parsing
         */
        bool parse(std::size_t size);

        error_code state() const;

        /*
         * getters for the header informations.
         * @pre a prior call to parse() returned true
         * @pre state() returns ok
         */
        unsigned                major_version() const;
        unsigned                minor_version() const;

        /**
         * @brief returns 1000* major_version() + minor_version()
         */
        unsigned                milli_version() const;

        /**
         * @brief the whole request text including the final empty line with trailing \\r\n
         */
        tools::substring        text() const;

        bool option_available(const char* header_name, const char* option) const;

        /**
         * @brief if there is a header with the given name a pointer to it will be returned.
         *
         * The returned object will become invalid, when the request_header, where the pointer 
         * is obtained from, becomes invalid. When searching for the corresponding header, is 
         * not case sensitiv. 
         * Trailing and leading \\r\n and tabs and spaces are removed in the returned value. If 
         * the header values spawns multiple lines, \\r\n within the value are _not_ removed.
         */
        const header* find_header(const char* header_name) const;

        /**
         * @brief returns true, if this is a 1.0 header, or in case of an 1.1 (or later) 
         * header, the "Connection : close" header was found
         */
        bool close_after_response() const;

        /** 
         * @brief filters the specified headers from the request and returns the result as sequence of tools::substring
         * for further sending 
         */
        std::vector<tools::substring> filtered_request_text(const http::filter&) const;

        /**
         * @brief returns true, if no single byte was received and buffered
         */
        bool empty() const;

    protected:
        message_base();

        /**
         * @brief constructs a new request_header with the remaining data past the
         * last read request header
         *
         * @attention after constructing a request header this way, it might be possible
         * that this header is too already complete.
         *
         * @param old_header the header that contains data, that doesn't belongs to the previous htt-header
         * @param remaining returns the unparsed bytes. If not 0, parse() can be called with this
         * information.
         */
        message_base(const Type& old_header, std::size_t& remaining, copy_trailing_buffer_t);

        /**
         * @brief constructs a new request_header from a text literal. This can be quit handy, for testing.
         */
        explicit message_base(const char*);

        ~message_base() {}

        bool parse_version(const tools::substring& version_text);

        // implementation of find_header that doesn't jet expects a fully, correctly parsed header, but instead 
        // searchs the header parsed to far
        const header* find_header_impl(const char* header_name) const;

    private:
        message_base(const message_base&);
        message_base& operator=(const message_base&);

        void crlf_found(const char* start, const char* end);
        void header_found(const char* start, const char* end);

        void parse_error();

        char                        buffer_[1024];
        std::size_t                 write_ptr_;
        std::size_t                 parse_ptr_; // already consumed including trailing CRLF
        std::size_t                 read_ptr_;  // read, but no CRLF found so far
        error_code                  error_;

        tools::substring            start_line_;
        unsigned                    major_version_;
        unsigned                    minor_version_;

        enum {
            expect_request_line,
            expect_header
        } parser_state_;

        typedef std::vector<header> header_list_t;
        header_list_t               headers_;
    };

} // namespace http 

#endif // include guard
