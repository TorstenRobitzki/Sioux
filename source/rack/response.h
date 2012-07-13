// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef RACK_RESPONSE_H
#define RACK_RESPONSE_H

#include "rack/ruby_land_queue.h"
#include "server/response.h"
#include <boost/smart_ptr/enable_shared_from_this.hpp>
#include <boost/system/error_code.hpp>
#include <boost/function.hpp>
#include <deque>
#include <vector>

namespace boost {
    namespace asio {
        class io_service;
    }
}

namespace http {
    class request_header;
}

namespace rack
{
    /**
     * @brief class responsible for reading a request, putting itself on a queue and finally send the response back
     */
    template < class Connection >
    class response : public boost::enable_shared_from_this< response< Connection > >, public server::async_response
    {
    public:
        typedef Connection connection_t;

        response(
            const boost::shared_ptr< connection_t >&                connection,
            const boost::shared_ptr< const http::request_header >&  request,
            boost::asio::io_service&                                queue,
            ruby_land_queue&                                        ruby_land );

    private:
        void start();

        /*
         * This function will call the given call back interface call() function with the body received
         * and the request header.
         */
        void call_application( application_interface& );

        void body_read_handler(
            const boost::system::error_code& error,
            const char* buffer,
            std::size_t bytes_read_and_decoded );

        void response_write_handler( const boost::system::error_code& error, std::size_t bytes_transferred );

        const boost::shared_ptr< connection_t >                 connection_;
        const boost::shared_ptr< const http::request_header >   request_;
        boost::asio::io_service&                                queue_;
        ruby_land_queue&                                        ruby_land_queue_;
        std::vector< char >                                     body_;
        std::vector< char >                                     response_;
        std::size_t                                             write_ptr_;
    };
}

#endif /* RACK_RESPONSE_H */
