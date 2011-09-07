// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SOURCE_HTTP_REQUEST_H
#define SOURCE_HTTP_REQUEST_H

#include "http/message.h"

namespace http
{
    namespace details
    {
        /**
         * data is extracted to an own base class, to be able to initialize it before the message_base<>
         * base class c'tor is invoked.
         */
        struct request_data 
        {
            request_data();

            http::http_method_code      method_;
            tools::substring            uri_;
            unsigned                    port_;
            tools::substring            host_;
            http_error_code             error_code_;
        };
    }

    /**
     * @brief access and parsing of http request headers
     */
    class request_header : private details::request_data, public message_base<request_header>
	{
    public:
        request_header();

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
         *
         * @pre state() returned ok
         */
        request_header(const request_header& old_header, std::size_t& remaining, copy_trailing_buffer_t);

        /**
         * @brief constructs a new request_header from a text literal. This can be quit handy, for testing.
         */
        explicit request_header(const char*);

        http::http_method_code  method() const;

        /**
         * @brief the requested, unmodified uri from the header
         */
        tools::substring        uri() const;

        /**
         * @brief the host from the host header field
         */
        tools::substring        host() const;

        /**
         * @brief the port from the host header field
         */
        unsigned                port() const;

        /**
         * @brief in case, that the state() function returns syntax_error, this function will 
         * deliver a more detailed error code.
         */
        http_error_code         error_code() const;

        /**
         * @brief returns true, if the header indicates the presents of a message body
         *
         * The presents of a message body is indicated by the presents of a Content-Length
         * or Transfer-Encoding header
         */
        bool                    body_expected() const;

    private:
        bool start_line_found(const char* start, const char* end);

        message::error_code end_of_request();

        // returns syntax_error and sets error_code_ to http_bad_request
        message::error_code bad_request();

        friend class message_base<request_header>;
	};

} // namespace http

#endif 

