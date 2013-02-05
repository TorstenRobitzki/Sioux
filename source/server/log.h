// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SERVER_LOG_H
#define SIOUX_SERVER_LOG_H

#include <boost/thread/mutex.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <ostream>

#include "http/request.h"
#include "tools/hexdump.h"

namespace server
{
	class async_response;

    /**
     * @brief log events to a stream
     */
    class stream_event_log
    {
    public:
        template < class Parameter >
        explicit stream_event_log( const Parameter& param )
            : mutex_()
            , out_( param.logstream() )
            , connection_cnt_( 0 )
        {
        }

        // uses std::cout as output stream
        stream_event_log();


        template <class Connection>
        void event_connection_created(const Connection&) 
        {
            boost::mutex::scoped_lock lock(mutex_);
            out_ << "new connection created: " << ++connection_cnt_ << std::endl;
        }

        template <class Connection>
        void event_connection_destroyed(const Connection&)
        {
            boost::mutex::scoped_lock lock(mutex_);
            out_ << "connection destroyed: " << --connection_cnt_ << std::endl;
        }

        template <class Connection, class Buffers, class Response>
        void event_data_write(const Connection&, const Buffers& b, const Response&)
        {
            const std::size_t size = std::distance(boost::asio::buffers_begin(b), boost::asio::buffers_end(b));

            boost::mutex::scoped_lock lock(mutex_);
            out_ << "event_data_write: " << size << std::endl;
        }

        template <class Connection, class Buffers, class Response>
        void event_writer_blocked(const Connection&, const Buffers&, const Response&)
        {
            boost::mutex::scoped_lock lock(mutex_);
            out_ << "event_writer_blocked" << std::endl;
        }

        template <class Connection, class Response>
        void event_response_completed(const Connection&, const Response&)
        {
            boost::mutex::scoped_lock lock(mutex_);
            out_ << "event_response_completed" << std::endl;
        }

        template <class Connection, class Response, class Ec>
        void event_response_not_possible(const Connection&, const Response&, const Ec& ec)
        {
            boost::mutex::scoped_lock lock(mutex_);
            out_ << "event_response_not_possible: " << ec << std::endl;
        }

        template <class Connection, class Response>
        void event_response_not_possible(const Connection&, const Response&)
        {
            boost::mutex::scoped_lock lock(mutex_);
            out_ << "event_response_not_possible" << std::endl;
        }

        template < class Connection >
        void event_keep_alive_timeout( const Connection& )
        {
            boost::mutex::scoped_lock lock(mutex_);
            out_ << "event_keep_alive_timeout" << std::endl;
        }

        template <class Connection>
        void event_shutdown_read(const Connection&)
        {
            boost::mutex::scoped_lock lock(mutex_);
            out_ << "event_shutdown_read" << std::endl;
        }

        template <class Connection>
        void event_shutdown_close(const Connection&)
        {
            boost::mutex::scoped_lock lock(mutex_);
            out_ << "event_shutdown_close" << std::endl;
        }

        /* 
         * Proxy events
         */
        template <class Connection, class Response>
        void event_proxy_response_started(const Connection&, const Response&)
        {
            boost::mutex::scoped_lock lock(mutex_);
            out_ << "event_proxy_response_started" << std::endl;
        }

        template <class Connection, class Response>
        void event_proxy_response_destroyed(const Connection&, const Response&)
        {
            boost::mutex::scoped_lock lock(mutex_);
            out_ << "event_proxy_response_destroyed" << std::endl;
        }

        template <class Connection, class Response, class Socket, class Ec>
        void event_proxy_orgin_connected(const Connection&, const Response&, const Socket* socket, const Ec& ec)
        {
            boost::mutex::scoped_lock lock(mutex_);
            out_ << "event_proxy_orgin_connected: socket: " << static_cast<const void*>(socket) << " ec: " << ec << std::endl;
        }

        template <class Connection, class Response, class Ec>
        void event_proxy_request_written(const Connection&, const Response&, const Ec& e, std::size_t bytes_transferred)
        {
            boost::mutex::scoped_lock lock(mutex_);
            out_ << "event_proxy_request_written: ec: " << e <<  " size: " << bytes_transferred << std::endl;
        }

        template <class Connection, class Response>
        void event_proxy_response_restarted(const Connection&, const Response&, unsigned tries)
        {
            boost::mutex::scoped_lock lock(mutex_);
            out_ << "event_proxy_response_restarted: try: " << tries << std::endl;
        }

        /*
         * request events
         */
        template < class Connection >
        void event_before_response_started( const Connection&, const http::request_header& request, const async_response& )
        {
            boost::mutex::scoped_lock lock(mutex_);
            out_ << "event_before_response_started: " << request_url( request ) << std::endl;
        }

        template < class Connection >
        void event_close_after_response( const Connection&, const http::request_header& request )
        {
            boost::mutex::scoped_lock lock(mutex_);
            out_ << "event_close_after_response: " << request_url( request ) << std::endl;
        }

        /*
         * listen / accept server logging
         */
        void event_accepting_new_connection( const boost::asio::ip::tcp::endpoint& local_endpoint,
            const boost::asio::ip::tcp::endpoint& remote_endpoint )
        {
            boost::mutex::scoped_lock lock(mutex_);
            out_ << "event_accepting_new_connection: local: " << local_endpoint << " remote: " << remote_endpoint
                << std::endl;
        }
    private:
        boost::mutex    mutex_;
        std::ostream&   out_;

        int             connection_cnt_;
    };

    class stream_error_log
    {
    public:
        template <class P>
        explicit stream_error_log(const P& p)
            : mutex_()
            , log_(p.logstream())
        {
        }

        // uses std::cerr as output stream
        stream_error_log();

        template < class Connection, class P1 >
        void log_error( const Connection&, const char* function_name, const P1& p1 )
        {
            boost::mutex::scoped_lock lock(mutex_);
            log_ << "in \"" << function_name << "\" p1: " << p1 << std::endl;
        }

        template <class Connection, class P1, class P2>
        void log_error(const Connection&, const char* function_name, const P1& p1, const P2& p2)
        {
            boost::mutex::scoped_lock lock(mutex_);
            log_ << "in \"" << function_name << "\" p1: " << p1 << " p2: " << p2 << std::endl;
        }

        template < class Connection >
        void error_request_parse_error( const Connection&, const http::request_header& request )
        {
            boost::mutex::scoped_lock lock(mutex_);
            log_ << "error_request_parse_error: " << request.state() << std::endl;
            tools::hex_dump( log_, request.text().begin(), request.text().end() );
            log_ << std::endl;
        }

        template < class Connection >
        void error_executing_request_handler( const Connection&, const http::request_header& request,
        		const std::string& error_text)
        {
            boost::mutex::scoped_lock lock(mutex_);
            log_ << "error_executing_request_handler: " << request_url( request ) << std::endl;
            log_ << "error: " << error_text << std::endl;
        }

        /*
         * listen / accept server logging
         */
        template < class Error >
        void error_accepting_new_connection( const boost::asio::ip::tcp::endpoint& local_endpoint,
            const Error& error )
        {
            boost::mutex::scoped_lock lock(mutex_);
            log_ << "error_accepting_new_connection: local: " << local_endpoint;
            log_ << "\nerror: " << error << std::endl;
        }
    private:
        boost::mutex    mutex_;
        std::ostream&   log_;
    };
}

#endif // include guard

