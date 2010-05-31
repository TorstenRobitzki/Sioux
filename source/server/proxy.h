// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_PROXY_H
#define SIOUX_SOURCE_SERVER_PROXY_H

#include "server/response.h"
#include "server/request.h"
#include "tools/dynamic_type.h"
#include <boost/shared_ptr.hpp>
#include <boost/asio/error.hpp>
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
         * @brief returns a modified host
         *
         * The default implementation simply returns the passed string
         */
        virtual std::string modified_host(const std::string& original_host);

        /**
         * @brief returns a modified url 
         *
         * The default implementation simply returns the passed string
         */
        virtual std::string modified_url(const std::string& original_url);

        /**
         * @brief returns a connection to talk to the server where a request should be forwarded to
         * @attention if handler is called with a valid connection, release_connection() have to be
         * called, when the connection isn't used anymore. If connection are not release, this will
         * leed to resouce leaks.
         */
        template <class Connection, class ConnectHandler>
        void async_get_proxy_connection(
            const std::string&  orgin,
            ConnectHandler      handler);

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
         * @param orgin the name of the orgin server that should be connected. If access is restricted or there is something wrong with the 
         * name, a proxy_error exception should be thrown
         * @param call_back the implemenation is owner of the passed object and should call connection_received() on the object,
         * when a connection could be established, or when an error occured. 
         */
        virtual void async_get_proxy_connection(
            const tools::dynamic_type&          connection_type,
            const std::string&                  orgin,
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

    /**
     * @brief forwards the request to an other server and answers with the answer of that server
     *
     */
    template <class Connection>
    class proxy_response : public async_response, 
                           public boost::enable_shared_from_this<proxy_response<Connection> >, 
                           private boost::noncopyable
    {
    public:
        proxy_response(const boost::shared_ptr<Connection>& connection, const boost::shared_ptr<const server::request_header>& header, proxy_config_base& config)
            : connection_(connection)
            , request_(header)
            , config_(config)
        {
        }

        ~proxy_response();

        void start();
    private:
        void handle_orgin_connect(
            typename Connection::socket_t*  orgin_socket,
            boost::system::error_code&      error);

        boost::shared_ptr<Connection>                   connection_;
        boost::shared_ptr<const server::request_header> request_;
        proxy_config_base&                              config_;

        char                                            buffer_[1024];
        std::string                                     host_line_;

        typename Connection::socket_t*                  socket_;
    };

    ////////////////////////////////////////
    // implemenation
    template <class Connection>
    proxy_response<Connection>::~proxy_response()
    {
        if ( socket_ )
            config_.release_connection(socket_);
    }

    template <class Connection>
    void proxy_response<Connection>::start()
    {
        /// @todo use correct host, handle missing host or invalid host
        config_.async_get_proxy_connection<Connection>(
            config_.modified_host("foobar"), 
            boost::bind(&proxy_response::handle_orgin_connect, shared_from_this(), _1, _2));

        
    }

    template <class Connection>
    void proxy_response<Connection>::handle_orgin_connect(
        typename Connection::socket_t*  orgin_socket,
        boost::system::error_code&      error)
    {
        if ( !error && orgin_socket )
        {   
            socket_ = orgin_socket;
        }
    }

} // namespace server 

#endif



