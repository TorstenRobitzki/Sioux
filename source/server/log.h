// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SERVER_LOG_H
#define SIOUX_SERVER_LOG_H

#include <boost/thread/mutex.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <ostream>

namespace server
{
    /**
     * @brief log events to a stream
     */
    class stream_event_log
    {
    public:
        template <class P>
        explicit stream_event_log(const P& p) 
            : mutex_()
            , out_(p.logstream())
            , connection_cnt_(0)
        {
        }

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

        template <class Connection, class P1, class P2>
        void log_error(const Connection&, const char* function_name, const P1& p1, const P2& p2)
        {
            boost::mutex::scoped_lock lock(mutex_);
            log_ << "in \"" << function_name << "\" p1: " << p1 << " p2: " << p2 << std::endl;
        }

    private:
        boost::mutex    mutex_;
        std::ostream&   log_;
    };
}

#endif // include guard

