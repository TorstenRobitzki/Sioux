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
        proxy_response(const boost::shared_ptr<Connection>& connection, const boost::shared_ptr<const http::request_header>& header, proxy_config_base& config)
            : connection_(connection)
            , request_(header)
            , config_(config)
            , error_close_(false)
            , proxy_socket_(0)
        {
        }

        ~proxy_response();

        void start();
    private:
        typedef report_error_guard<Connection>  error_guard;

        void done();

        void handle_orgin_connect(
            typename Connection::socket_t*      orgin_socket,
            const boost::system::error_code&    error);
    
        void request_written(
            const boost::system::error_code&    error,
            std::size_t                         bytes_transferred); 

        // checks the request_header for errors that will prevent further processing
        // returns true, if there is no reason to furth process the request.
        // If the function returns false, 
        // @todo what happens to a body, if any, appended to the header
        bool check_header();

        void error_written(
            const boost::system::error_code&    error,
            std::size_t                         bytes_transferred); 

        void bad_gateway();

        boost::shared_ptr<Connection>                   connection_;
        boost::shared_ptr<const http::request_header>   request_;
        proxy_config_base&                              config_;

        std::vector<tools::substring>                   outbuffers_;;

        std::string                                     error_response_;
        // indicates, that connection should be closed
        bool                                            error_close_;

        typename Connection::socket_t*                  proxy_socket_;
    };

    ////////////////////////////////////////
    // proxy_config_base implementation
    template <class Connection, class ConnectHandler>
    void proxy_config_base::async_get_proxy_connection(
        const std::string&  orgin,
        ConnectHandler      handler)
    {
        std::auto_ptr<connect_callback> cb(new callback<ConnectHandler, Connection>(handler));
        async_get_proxy_connection(typeid(Connection), orgin, cb);
    }

    template <class Connection>
    void proxy_config_base::release_connection(Connection* c)
    {
        release_connection(typeid(Connection), c);
    }


    ////////////////////////////////////////
    // proxy_response implementation
    template <class Connection>
    proxy_response<Connection>::~proxy_response()
    {
        if ( proxy_socket_ )
            config_.release_connection(proxy_socket_);
    }

    template <class Connection>
    void proxy_response<Connection>::start()
    {
        connection_->response_started(shared_from_this());

        if ( !check_header() )
        {
            connection_->async_write_some(
                outbuffers_,
                boost::bind(&proxy_response::error_written, shared_from_this(), 
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred),
                *this);

            return;
        }

        /// @todo use correct host, handle missing host or invalid host
        config_.async_get_proxy_connection<Connection::socket_t>(
            config_.modified_host("foobar"), 
            boost::bind(&proxy_response::handle_orgin_connect, shared_from_this(), _1, _2));

        // while waiting for the response, the request to the orgin server can be assembled
        static const http::filter general_unused_headers("connection, keep-alive");
        http::filter    unused_headers(general_unused_headers);

        const http::header* const connection_header = request_->find_header("connection");
        
        if ( connection_header != 0 )
        {
            unused_headers += http::filter(connection_header->value());
        }

        outbuffers_ = request_->filtered_request_text(unused_headers);
    }

    template <class Connection>
    void proxy_response<Connection>::done()
    {
        if ( error_close_ )
            connection_->shutdown_close();

        connection_->response_completed(*this);
    }

    template <class Connection>
    void proxy_response<Connection>::handle_orgin_connect(
        typename Connection::socket_t*      orgin_socket,
        const boost::system::error_code&    error)
    {
        error_guard fail(*connection_, *this, http::http_bad_gateway);

        if ( !error && orgin_socket )
        {   
            proxy_socket_ = orgin_socket;

            /// @todo add timeout
            proxy_socket_->async_write_some(
                outbuffers_, 
                boost::bind(
                    &proxy_response::request_written, 
                    shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));

            fail.dismiss();
        } 
    }
        
    template <class Connection>
    void proxy_response<Connection>::request_written(
        const boost::system::error_code&    /*error*/,
        std::size_t                         /*bytes_transferred*/)
    {
        /** @todo */
        done();
    }

    template <class Connection>
    bool proxy_response<Connection>::check_header()
    {
        return true;
    }

    template <class Connection>
    void proxy_response<Connection>::error_written(
        const boost::system::error_code&    /* error */,
        std::size_t                         /* bytes_transferred */)
    {
        done();
    }

} // namespace server 

#endif



