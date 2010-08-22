// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_PROXY_RESPONSE_H
#define SIOUX_SOURCE_SERVER_PROXY_RESPONSE_H

#include "server/response.h"
#include "server/proxy.h"
#include "server/transfer_buffer.h"
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
    template <class Connection, std::size_t BodyBufferSize = 1024>
    class proxy_response : public async_response, 
                           public boost::enable_shared_from_this<proxy_response<Connection, BodyBufferSize> >, 
                           private boost::noncopyable
    {
    public:
        proxy_response(const boost::shared_ptr<Connection>& connection, const boost::shared_ptr<const http::request_header>& header, proxy_connector_base& connector)
            : connection_(connection)
            , request_(header)
            , connector_(connector)
            , outbuffers_()
            , proxy_socket_(0)
            , response_body_exists_(false)
            , reading_body_from_orgin_(false)
            , writing_body_to_client_(false)
            , restart_counter_(0)
        {
            if ( request_->body_expected() )
                throw std::runtime_error("Request-Body in Proxy currently not implemented");
        }

        ~proxy_response();

    private:
        // async_response implementation
        void start();

        // if an error occured, while communication with the orgin server 
        // returns true, if a restart is issued
        bool restart();

        // make sure, that after a read-error from the proxy occured, the connection to the proxy will not be restarted.
        void disable_restart();

        typedef report_error_guard<Connection>  error_guard;
        typedef close_connection_guard<Connection> close_guard;

        void handle_orgin_connect(
            typename Connection::socket_t*      orgin_socket,
            const boost::system::error_code&    error);
    
        void request_written(
            const boost::system::error_code&    error,
            std::size_t                         bytes_transferred); 

        void handle_read_from_orgin(
            const boost::system::error_code&    error,
            std::size_t                         bytes_transferred); 

        void response_header_written(
            const boost::system::error_code&    error,
            std::size_t                         bytes_transferred); 

        void handle_body_written(
            const boost::system::error_code&    error,
            std::size_t                         bytes_transferred);

        void issue_read(const std::pair<char*, std::size_t>& buffer);
        void issue_read(const boost::asio::mutable_buffers_1& buffer);

        void issue_write(const boost::asio::const_buffers_1& buffer);

        void forward_header();

        template <class Header>
        std::vector<tools::substring> filtered_header(const Header& header);

        boost::shared_ptr<Connection>                   connection_;
        boost::shared_ptr<const http::request_header>   request_;
        proxy_connector_base&                           connector_;

        std::vector<tools::substring>                   outbuffers_;

        typename Connection::socket_t*                  proxy_socket_;

        http::response_header                           response_header_from_proxy_;
        transfer_buffer<BodyBufferSize>                 body_buffer_;
    
        bool                                            response_body_exists_;

        // keep track of whether a read or write is already issued
        bool                                            reading_body_from_orgin_;
        bool                                            writing_body_to_client_;

        unsigned                                        restart_counter_;

        static const http::filter                       connection_headers_to_be_removed_;
        static const unsigned                           max_restarts_ = 3;
    };

    ////////////////////////////////////////
    // proxy_response implementation
    template <class Connection, std::size_t BodyBufferSize>
    proxy_response<Connection, BodyBufferSize>::~proxy_response()
    {
        connection_->trait().event_proxy_response_destroyed(*connection_, *this);

        if ( proxy_socket_ )
            connector_.dismiss_connection(proxy_socket_);

        connection_->response_completed(*this);
    }

    template <class Connection, std::size_t BodyBufferSize>
    void proxy_response<Connection, BodyBufferSize>::start()
    {
        connection_->trait().event_proxy_response_started(*connection_, *this);

        connector_.async_get_proxy_connection<Connection::socket_t>(
            request_->host(), request_->port(),
            boost::bind(&proxy_response::handle_orgin_connect, shared_from_this(), _1, _2));

        // while waiting for the response, the request to the orgin server can be assembled
        outbuffers_ = filtered_header(*request_);
    }

    template <class Connection, std::size_t BodyBufferSize>
    bool proxy_response<Connection, BodyBufferSize>::restart()
    {
        ++restart_counter_;
        connection_->trait().event_proxy_response_restarted(*connection_, *this, restart_counter_);

        if ( restart_counter_ > max_restarts_ )
            return false;

        if ( proxy_socket_ )
            connector_.dismiss_connection(proxy_socket_);

        proxy_socket_               = 0;
        reading_body_from_orgin_    = false;
        writing_body_to_client_     = false;

        connector_.async_get_proxy_connection<Connection::socket_t>(
            request_->host(), request_->port(),
            boost::bind(&proxy_response::handle_orgin_connect, shared_from_this(), _1, _2));

        return true;
    }

    template <class Connection, std::size_t BodyBufferSize>
    void proxy_response<Connection, BodyBufferSize>::disable_restart()
    {
        restart_counter_ = max_restarts_;
    }

    template <class Connection, std::size_t BodyBufferSize>
    void proxy_response<Connection, BodyBufferSize>::handle_orgin_connect(
        typename Connection::socket_t*      orgin_socket,
        const boost::system::error_code&    error)
    {
        connection_->trait().event_proxy_orgin_connected(*connection_, *this, orgin_socket, error);

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
        
    template <class Connection, std::size_t BodyBufferSize>
    void proxy_response<Connection, BodyBufferSize>::request_written(
        const boost::system::error_code&    error,
        std::size_t                         bytes_transferred)
    {
        connection_->trait().event_proxy_request_written(*connection_, *this, error, bytes_transferred);

        error_guard fail(*connection_, *this, http::http_bad_gateway);

        if ( !error )
        {
            // now starting to read the message header from the proxy
            issue_read(response_header_from_proxy_.read_buffer());
            fail.dismiss();
        }
    }

    template <class Connection, std::size_t BodyBufferSize>
    void proxy_response<Connection, BodyBufferSize>::handle_read_from_orgin(
        const boost::system::error_code&    error,
        std::size_t                         bytes_transferred)
    {
        error_guard fail(*connection_, *this, http::http_bad_gateway);
        reading_body_from_orgin_ = false;

        if ( error )
        {
            connection_->trait().log_error(*this, "proxy_response::handle_read_from_orgin", error, bytes_transferred);

            if ( restart() )
                fail.dismiss();

            return;
        }

        disable_restart();

        if ( response_header_from_proxy_.state() == http::message::parsing && bytes_transferred != 0 )
        {
            if ( response_header_from_proxy_.parse(bytes_transferred) )
            {
                if ( response_header_from_proxy_.state() == http::message::ok )
                {
                    forward_header();

                    response_body_exists_ = response_header_from_proxy_.body_expected(request_->method());

                    if ( response_body_exists_ )        
                    {
                        body_buffer_.start(response_header_from_proxy_);
                        issue_read(body_buffer_.write_buffer());
                        issue_write(body_buffer_.read_buffer());
                    }

                    fail.dismiss();
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
            body_buffer_.data_written(bytes_transferred);
            issue_read(body_buffer_.write_buffer());
            issue_write(body_buffer_.read_buffer());
 
            fail.dismiss();
        }
    }

    template <class Connection, std::size_t BodyBufferSize>
    void proxy_response<Connection, BodyBufferSize>::response_header_written(
        const boost::system::error_code&    error,
        std::size_t                         bytes_transferred)
    {
        // if writing failed, there is hardly any way to reply with a response, saying so
        close_guard close_connection(*connection_, *this);

        if ( !error && bytes_transferred != 0 )
        {
            if ( response_body_exists_ )
            {
                writing_body_to_client_ = false;
                issue_read(body_buffer_.write_buffer());
                issue_write(body_buffer_.read_buffer());
            }
            else
            {
                // all is done here!
                connector_.release_connection(proxy_socket_, response_header_from_proxy_);
                proxy_socket_ = 0;
            }

            close_connection.dismiss();
        }
    }

    template <class Connection, std::size_t BodyBufferSize>
    void proxy_response<Connection, BodyBufferSize>::handle_body_written(
        const boost::system::error_code&    error,
        std::size_t                         bytes_transferred)
    {
        // if writing failed, there is hardly any way to reply with a response, saying so
        close_guard close_connection(*connection_, *this);
        writing_body_to_client_ = false;

        if ( !error )
        {
            body_buffer_.data_read(bytes_transferred);

            if ( body_buffer_.transmission_done() )
            {
                // all is done here!
                connector_.release_connection(proxy_socket_, response_header_from_proxy_);
                proxy_socket_ = 0;
            }
            else
            {
                issue_read(body_buffer_.write_buffer());
                issue_write(body_buffer_.read_buffer());
            }

            close_connection.dismiss();
        }
    }

    template <class Connection, std::size_t BodyBufferSize>
    void proxy_response<Connection, BodyBufferSize>::issue_read(const std::pair<char*, std::size_t>& buffer)
    {
        issue_read(boost::asio::buffer(buffer.first, buffer.second));
    }

    template <class Connection, std::size_t BodyBufferSize>
    void proxy_response<Connection, BodyBufferSize>::issue_read(const boost::asio::mutable_buffers_1& buffer)
    {
        if ( !reading_body_from_orgin_ && buffer_size(buffer) != 0 )
        {
            reading_body_from_orgin_ = true;

            proxy_socket_->async_read_some(
                buffer,
                boost::bind(&proxy_response::handle_read_from_orgin, 
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred)
            );
        }
    }

    template <class Connection, std::size_t BodyBufferSize>
    void proxy_response<Connection, BodyBufferSize>::issue_write(const boost::asio::const_buffers_1& buffer)
    {
        if ( !writing_body_to_client_ && buffer_size(buffer) != 0 )
        {
            writing_body_to_client_ = true;
            connection_->async_write_some(
                buffer,
                boost::bind(&proxy_response::handle_body_written,
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred),
                *this
            );
        }
    }

    template <class Connection, std::size_t BodyBufferSize>
    void proxy_response<Connection, BodyBufferSize>::forward_header()
    {
        // filter all connection headers
        outbuffers_ = filtered_header(response_header_from_proxy_);
        writing_body_to_client_ = true;

        connection_->async_write(
                outbuffers_, 
                boost::bind(
                    &proxy_response::response_header_written, 
                    shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred),
                *this);
    }

    template <class Connection, std::size_t BodyBufferSize>
    template <class Header>
    std::vector<tools::substring> proxy_response<Connection, BodyBufferSize>::filtered_header(const Header& header)
    {
        http::filter                    unused_headers(connection_headers_to_be_removed_);

        const http::header* const connection_header = header.find_header("connection");
        
        if ( connection_header != 0 )
        {
            unused_headers += http::filter(connection_header->value());
        }

        return header.filtered_request_text(unused_headers);
    }

    template <class Connection, std::size_t BodyBufferSize>
    const http::filter proxy_response<Connection, BodyBufferSize>::connection_headers_to_be_removed_("connection, keep-alive");
} // namespace server

#endif 