// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_PROXY_RESPONSE_H
#define SIOUX_SOURCE_SERVER_PROXY_RESPONSE_H

#include "server/response.h"
#include "server/proxy.h"
#include "http/request.h"
#include "http/response.h"
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio/write.hpp>

namespace server
{
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
            , outbuffers_()
            , proxy_socket_(0)
        {
        }

        ~proxy_response();

    private:
        // async_response implementation
        void start();

        typedef report_error_guard<Connection>  error_guard;

        void handle_orgin_connect(
            typename Connection::socket_t*      orgin_socket,
            const boost::system::error_code&    error);
    
        void request_written(
            const boost::system::error_code&    error,
            std::size_t                         bytes_transferred); 

        void handle_read_from_orgin(
            const boost::system::error_code&    error,
            std::size_t                         bytes_transferred); 

        void issue_read(const std::pair<char*, std::size_t>& buffer);

        boost::shared_ptr<Connection>                   connection_;
        boost::shared_ptr<const http::request_header>   request_;
        proxy_config_base&                              config_;

        std::vector<tools::substring>                   outbuffers_;

        typename Connection::socket_t*                  proxy_socket_;

        http::response_header                           response_header_from_proxy_;
    };

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
        config_.async_get_proxy_connection<Connection::socket_t>(
            request_->host(), request_->port(),
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
    void proxy_response<Connection>::handle_orgin_connect(
        typename Connection::socket_t*      orgin_socket,
        const boost::system::error_code&    error)
    {
        error_guard fail(*connection_, *this, http::http_bad_gateway);

        if ( !error && orgin_socket )
        {   
            proxy_socket_ = orgin_socket;

            /// @todo add timeout
            boost::asio::async_write(
                *proxy_socket_,
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
        const boost::system::error_code&    error,
        std::size_t                         /* bytes_transferred */)
    {
        error_guard fail(*connection_, *this, http::http_bad_gateway);

        if ( !error )
        {
            issue_read(response_header_from_proxy_.read_buffer());
            fail.dismiss();
        }
    }

    template <class Connection>
    void proxy_response<Connection>::handle_read_from_orgin(
        const boost::system::error_code&    error,
        std::size_t                         bytes_transferred)
    {
        error_guard fail(*connection_, *this, http::http_bad_gateway);

        if ( error )
            return;

        if ( response_header_from_proxy_.state() == http::message::parsing )
        {
            if ( response_header_from_proxy_.parse(bytes_transferred) )
            {
                if ( response_header_from_proxy_.state() == http::message::ok )
                {
                }

            }
            else
            {
                issue_read(response_header_from_proxy_.read_buffer());
                fail.dismiss();
            }
        }
        else if ( response_header_from_proxy_.state() == http::message::ok )
        {
            /// @todo
        }
    }

    template <class Connection>
    void proxy_response<Connection>::issue_read(const std::pair<char*, std::size_t>& buffer)
    {
        proxy_socket_->async_read_some(
            boost::asio::buffer(buffer.first, buffer.second),
            boost::bind(&proxy_response::handle_read_from_orgin, 
                    shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred)
        );
    }

} // namespace server

#endif 