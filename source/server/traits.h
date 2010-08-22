// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_TRAITS_H
#define SIOUX_SOURCE_SERVER_TRAITS_H

namespace http 
{
    class request_header;
}

namespace server 
{

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
    };

    struct null_error_logger
    {
        template <class T>
        null_error_logger(const T&) {}

        null_error_logger() {}

        template <class Connection, class P1, class P2>
        void log_error(const Connection&, const char*, const P1&, const P2&)
        {
        }
    };

    namespace details 
    {
        struct null {};
    }

    /**
     * @brief interface to costumize different aspects of handling connections, requests and responses
     */
    template <class Network,
              class ResponseFactory,
              class EventLog = null_event_logger,
              class ErrorLog = null_error_logger>
    class connection_traits : 
        public ResponseFactory,
        public EventLog,
        public ErrorLog
    {
    public:
        connection_traits() {}

        /**
         * @brief constructor, that uses a parameters class to pass construction parameters to the different traits
         */
        template <class Parameters>
        explicit connection_traits(const Parameters& p)
            : ResponseFactory(p)
            , EventLog(p)
            , ErrorLog(p)
        {
        }

    private:
        connection_traits(const connection_traits&);
    };
}

#endif // include guard

