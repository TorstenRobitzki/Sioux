// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SOURCE_SERVER_REQUEST_H
#define SOURCE_SERVER_REQUEST_H

#include "http/http.h"
#include <boost/asio/buffer.hpp>

namespace server
{
    /**
     * @brief access and parsing of request headers
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
         */
        request_header(const request_header& old_header, copy_trailing_buffer_t);

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
         * success, or by any error. The passes size is set to the number of still unconsumed
         * bytes.
         */
        bool parse(std::size_t& size);

        enum error_code
        {
            ok,
            buffer_full,
            syntax_error,
            parsing
        };

        error_code state() const;
    private:
        char            buffer_[1024];
        std::size_t     write_ptr_;
        std::size_t     parse_ptr_;
        error_code      error_;
	};

} // namespace server

#endif 

