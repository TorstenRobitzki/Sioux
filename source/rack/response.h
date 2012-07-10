// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef RACK_RESPONSE_H
#define RACK_RESPONSE_H

#include "server/response.h"
#include <boost/smart_ptr/enable_shared_from_this.hpp>
#include <boost/system/error_code.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
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
     * @brief interface of the rack application
     */
    class call_back_interface
    {
    public:
        virtual std::vector< char > call( const std::vector< char >& body, const http::request_header& request ) = 0;
        virtual ~call_back_interface() {}
    };

    /**
     * @brief queue used to send complete received requests to the ruby part of the process
     *
     * This is some kind of workaround as there seems to be no reliable way to call a ruby callback.
     * (using rb_thread_call_with_gvl() results in undefined symbols on OSX with 1.9.2)
     */
    template < class T >
    class call_queue
    {
    public:
        call_queue();

        /**
         * @brief this function can be called without having the gvl (global vm lock) at hand
         */
        void push( const boost::shared_ptr< T >& );

        /**
         * @brief indicates that the process should shut down
         */
        void stop();

        /**
         * @brief this function blocks and releases the gvl while waiting for new work to come
         */
        void process_request( call_back_interface& );

        // internal
        void wait( boost::unique_lock<boost::mutex>& lock );
    private:

        boost::mutex        mutex_;
        boost::condition    condition_;
        bool                stop_flag_;
        typedef std::deque< boost::shared_ptr< T > > queue_t;
        queue_t             queue_;
    };

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
            call_queue< response< Connection > >&                   call_queue);

        /**
         * This function will call the given call back interface call() function with the body received
         * and the request header.
         */
        void call_application( call_back_interface& );

        void send_response();
    private:
        void start();

        void body_read_handler(
            const boost::system::error_code& error,
            const char* buffer,
            std::size_t bytes_read_and_decoded );

        void response_write_handler( const boost::system::error_code& error, std::size_t bytes_transferred );

        const boost::shared_ptr< connection_t >                 connection_;
        const boost::shared_ptr< const http::request_header >   request_;
        boost::asio::io_service&                                queue_;
        call_queue< response< Connection > >&                   call_queue_;
        std::vector< char >                                     body_;
        std::vector< char >                                     response_;
        std::size_t                                             write_ptr_;
    };
}

#endif /* RACK_RESPONSE_H */
