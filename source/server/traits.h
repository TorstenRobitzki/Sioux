// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_TRAITS_H
#define SIOUX_SOURCE_SERVER_TRAITS_H

#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace http 
{
    class request_header;
}

namespace server 
{

	/**
	 * @brief prototype for all currently known events.
	 *
	 * This implementation simply does nothing when an event occurs.
	 */
    struct null_event_logger
    {
        null_event_logger() {}

        template <class T>
        null_event_logger(const T&) {}

        template <class T>
        void event_connection_created(const T&) {}

        template <class T>
        void event_connection_destroyed(const T&) {}

        template <class Connection, class Buffers, class Response>
        void event_data_write(const Connection&, const Buffers&, const Response&) {}

        template <class Connection, class Buffers, class Response>
        void event_writer_blocked(const Connection&, const Buffers&, const Response&) {}

        template <class Connection, class Response>
        void event_response_completed(const Connection&, const Response&) {}

        template <class Connection, class Response, class Ec>
        void event_response_not_possible(const Connection&, const Response&, const Ec&) {}

        template <class Connection, class Response>
        void event_response_not_possible(const Connection&, const Response&) {}

        template < class Connection >
        void event_keep_alive_timeout( const Connection& ) {}

        template <class Connection>
        void event_shutdown_read(const Connection&) {}

        template <class Connection>
        void event_shutdown_close(const Connection&) {}

        /* 
         * Proxy events
         */
        template <class Connection, class Response>
        void event_proxy_response_started(const Connection&, const Response&) {}

        template <class Connection, class Response>
        void event_proxy_response_destroyed(const Connection&, const Response&) {}

        template <class Connection, class Response, class Socket, class Ec>
        void event_proxy_orgin_connected(const Connection&, const Response&, const Socket*, const Ec&) {}

        template <class Connection, class Response, class Ec>
        void event_proxy_request_written(const Connection&, const Response&, const Ec&, std::size_t) {}

        template <class Connection, class Response>
        void event_proxy_response_restarted(const Connection&, const Response&, unsigned) {}

        /*
         * request events
         */
        template < class Connection, class Request, class ResponseHandler >
        void event_before_response_started( const Connection&, const Request&, const ResponseHandler& ) {}

        template < class Connection, class Request >
        void event_close_after_response( const Connection&, const Request& ) {}

        /*
         * listen / accept server logging
         */
        template < class EndPoint >
        void event_accepting_new_connection( const EndPoint& local_endpoint, const EndPoint& remote_endpoint ) {}
    };

    struct null_error_logger
    {
        template <class T>
        null_error_logger(const T&) {}

        null_error_logger() {}

        template < class Connection, class P1 >
        void log_error( const Connection&, const char*, const P1& p1 )
        {
        }

        template <class Connection, class P1, class P2>
        void log_error(const Connection&, const char*, const P1&, const P2&)
        {
        }

        template < class Connection, class Request >
        void error_request_parse_error( const Connection&, const Request& ) {}

        template < class Connection, class Request, class ErrorText >
        void error_executing_request_handler( const Connection&, const Request&, const ErrorText& ) {}

        /*
         * listen / accept server logging
         */
        template < class EndPoint, class Error >
        void error_accepting_new_connection( const EndPoint& local_endpoint, const Error& error ) {}
    };

    namespace details 
    {
        struct null {};
    }

    /**
     * @brief default connection configurations
     */
    class connection_config
    {
    public:
        /**
         * @brief returns an idle timeout of 30 seconds
         */
        boost::posix_time::time_duration keep_alive_timeout() const;

        /**
         * @brief returns a connection timeout of 3 seconds
         * @todo lookup reasonable values from the appache documentation
         */
        boost::posix_time::time_duration timeout() const;

        /**
         * @brief when accepting a new connection fails, the next attempt to accept a new connection
         *        is not made before this timeout have been reached.
         */
        boost::posix_time::time_duration reaccept_timeout() const;
    };

    /**
     * @brief interface to customize different aspects of handling connections, requests and responses
     *
     * EventLog is meant to log events for performance analyzing purposes and to count events.
     * ErrorLog is used to log errors, that can steam from misbehaving clients or from configuration problems.
     */
    template < class Network,
               class Timer,
               class ResponseFactory,
               class EventLog = null_event_logger,
               class ErrorLog = null_error_logger,
               class Configuration = connection_config >
    class connection_traits : 
        public ResponseFactory,
        public EventLog,
        public ErrorLog,
        public Configuration
    {
    public:
        /**
         * the underlying network stream type
         */
        typedef Network network_stream_type;

        /**
         * the timer to be used for timeouts
         */
        typedef Timer timeout_timer_type;

        connection_traits() {}

        /**
         * @brief constructor, that uses a parameters class to pass construction parameters to the different traits
         */
        template < class Parameters >
        explicit connection_traits( const Parameters& p )
            : ResponseFactory( p )
            , EventLog( p )
            , ErrorLog( p )
        {
        }

    private:
        connection_traits(const connection_traits&);
    };
}

#endif // include guard

