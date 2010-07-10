// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_PROXY_H
#define SIOUX_SOURCE_SERVER_PROXY_H

#include "server/response.h"
#include "http/request.h"
#include "tools/dynamic_type.h"
#include "http/filter.h"
#include <boost/shared_ptr.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/utility.hpp>
#include <string>
#include <memory>

namespace server 
{
    /** 
     * @brief exception, that is used to indicate problems when attemding to connect or
     * communicate with the target.
     */
    class proxy_error : public std::runtime_error
    {
    public:
        /**
         * @brief constructs a proxy_error with a message text
         */
        explicit proxy_error(const std::string&);
    };

    /**
     * @brief base class for a proxy configuration
     */
    class proxy_config_base
    {
    public:
        virtual ~proxy_config_base() {}

        /**
         * @brief returns a connection to talk to the server where a request should be forwarded to
         * @attention if handler is called with a valid connection, release_connection() have to be
         * called, when the connection isn't used anymore. If connection are not release, this will
         * leed to resouce leaks.
         * @param orgin_host the orign host from the request header
         * @param orgin_port the orgin port from the request header
         */
        template <class Connection, class ConnectHandler>
        void async_get_proxy_connection(
            const tools::substring& orgin_host,
            unsigned                orgin_port,
            ConnectHandler          handler);

        /** 
         * @brief releases a prior obtained connection
         * @pre the passed connection must be obtained by a call to async_get_proxy_connection()
         */
        template <class Connection>
        void release_connection(Connection*);
    protected:

        /**
         * @brief base class for storing the ConnectHandler passed to ConnectHandler
         */
        class connect_callback
        {
        public:
            virtual ~connect_callback() {}

            virtual void connection_received(void* connection, const boost::system::error_code& error) = 0;
        };

        /**
         * @brief interface to pass a call_back to the implementation
         *
         * @param connection_type an implementation have to check connection_type and response with an assert or exception, if no 
         * such type will be provided as a connection.
         * @param orgin_host the name of the orgin server that should be connected. If access is restricted or there is something wrong with the 
         * name, a proxy_error exception should be thrown
         * @param orgin_port the orgin port from the request header
         * @param call_back the implemenation is owner of the passed object and should call connection_received() on the object,
         * when a connection could be established, or when an error occured. 
         */
        virtual void async_get_proxy_connection(
            const tools::dynamic_type&          connection_type,
            const tools::substring&             orgin_host,
            unsigned                            orgin_port,
            std::auto_ptr<connect_callback>&    call_back) = 0;

        /**
         * @brief will be called a connection isn't used anymore
         *
         * @param connection_type an implementation have to check connection_type and response with an assert or exception, if no 
         * such type will be provided as a connection.
         * @param connection a pointer to the connection. 
         */
        virtual void release_connection(
            const tools::dynamic_type&          connection_type,
            void*                               connection) = 0;

    private:
        template <class Handler, class Connection>
        class callback : public connect_callback
        {
        public:
            callback(const Handler& h) : handler_(h) {}

        private:
            virtual void connection_received(void* connection, const boost::system::error_code& error)
            {
                handler_(static_cast<Connection*>(connection), error);
            }

            Handler handler_;
        };


    };

    ////////////////////////////////////////
    // proxy_config_base implementation
    template <class Connection, class ConnectHandler>
    void proxy_config_base::async_get_proxy_connection(
        const tools::substring& orgin_host,
        unsigned                orgin_port,
        ConnectHandler          handler)
    {
        std::auto_ptr<connect_callback> cb(new callback<ConnectHandler, Connection>(handler));
        async_get_proxy_connection(typeid(Connection), orgin_host, orgin_port, cb);
    }

    template <class Connection>
    void proxy_config_base::release_connection(Connection* c)
    {
        release_connection(typeid(Connection), c);
    }



} // namespace server 

#endif



