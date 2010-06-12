// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SOURCE_HTTP_RESPONSE_H
#define SOURCE_HTTP_RESPONSE_H

#include "http/message.h"

namespace http {

    namespace details {
        struct response_data
        {
            http::http_error_code       code_;
            tools::substring            phrase_;
        };
    }

    class response_header : private details::response_data, public message_base<response_header>
    {
    public:
        /**
         * @brief constructs a new response_header with the remaining data past the
         * last read response header
         *
         * @attention after constructing a response header this way, it might be possible
         * that this header is too already complete.
         *
         * @param old_header the header that contains data, that doesn't belongs to the previous htt-header
         * @param remaining returns the unparsed bytes. If not 0, parse() can be called with this
         * information.
         *
         * @pre state() returned ok
         */
        response_header(const response_header& old_header, std::size_t& remaining, copy_trailing_buffer_t);

        /**
         * @brief constructs a new request_header from a text literal. This can be quit handy, for testing.
         */
        explicit response_header(const char*);

        /** 
         * @brief returns the status code of the response
         * @pre state() returned ok
         */
        http::http_error_code   code() const;

        tools::substring        phrase() const;
    private:
        bool start_line_found(const char* start, const char* end);


        friend message_base<response_header>;
    };

} // namespace http

#endif // include guard


