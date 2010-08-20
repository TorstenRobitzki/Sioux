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
        void new_request(const http::request_header&)
        {
        }
    };

    struct null_error_logger
    {
    };

    /**
     * @brief interface to costumize different aspects of handling connections, requests and responses
     */
    template <class Network,
              class ResponseFactory,
              class EventLog = null_event_logger,
              class ErrorLog = null_error_logger>
    class connection_traits : 
        public ResponseFactory
    {
    public:
        connection_traits() {}

        ErrorLog& error_log()
        {
            return error_log_;
        }

        EventLog& event_log()
        {
            return event_log_;
        }

    private:
        connection_traits(const connection_traits&);
        ErrorLog    error_log_;
        EventLog    event_log_;
    };
}

#endif // include guard

