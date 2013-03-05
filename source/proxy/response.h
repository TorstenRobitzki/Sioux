// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_PROXY_RESPONSE_H
#define SIOUX_SOURCE_SERVER_PROXY_RESPONSE_H

#include "proxy/connector.h"
#include "server/response.h"
#include "server/transfer_buffer.h"
#include "server/timeout.h"
#include "server/error_code.h"
#include "http/request.h"
#include "http/response.h"
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace proxy
{
    /**
     * @brief forwards the request to an other server and answers with the answer of that server
     */
    template < class Connection, std::size_t BodyBufferSize = 1024 >
    class response : public server::async_response,
                     public boost::enable_shared_from_this< response< Connection, BodyBufferSize > >,
                     private boost::noncopyable
    {
    public:
        /**
         * @brief constructs a new response
         *
         * @param connection the connection where the request was read from and where the response should be written to
         * @param header request header read from the connection
         * @param connector connector to obtain a connection to the orgin server 
         * @param queue io_service, used to implement the timeouts while communicating with the orgin server
         * @param config a reference to the configuration currently in use
         */
        response(
            const boost::shared_ptr<Connection>&                    connection, 
            const boost::shared_ptr<const http::request_header>&    header, 
            connector_base&                                         connector,
            boost::asio::io_service&                                queue,
            const boost::shared_ptr< const configuration >&         config)
            : connection_(connection)
            , request_(header)
            , connector_(connector)
            , outbuffers_()
            , proxy_socket_(0)
            , response_body_exists_(false)
            , reading_body_from_orgin_(false)
            , writing_body_to_client_(false)
            , restart_counter_(0)
            , orgin_time_out_(config->orgin_timeout())
            , read_timer_(queue)
            , write_timer_(queue)
        {
            if ( request_->body_expected() )
                throw std::runtime_error("Request-Body in Proxy currently not implemented");
        }

        ~response();

    private:
        // async_response implementation
        void start();
        const char* name() const;

        // if an error occured, while communication with the orgin server 
        // returns true, if a restart is issued
        bool restart();

        // make sure, that after a read-error from the proxy occured, the connection to the proxy will not be restarted.
        void disable_restart();

        typedef server::report_error_guard< Connection >  error_guard;
        typedef server::close_connection_guard< Connection > close_guard;

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

        void orgin_timeout(const boost::system::error_code& error);

        void forward_header();

        template <class Header>
        std::vector<tools::substring> filtered_header(const Header& header);

        boost::shared_ptr<Connection>                   connection_;
        boost::shared_ptr<const http::request_header>   request_;
        connector_base&                                 connector_;

        std::vector<tools::substring>                   outbuffers_;

        typename Connection::socket_t*                  proxy_socket_;

        http::response_header                           response_header_from_proxy_;
        server::transfer_buffer< BodyBufferSize >       body_buffer_;
    
        bool                                            response_body_exists_;

        // keep track of whether a read or write is already issued
        bool                                            reading_body_from_orgin_;
        bool                                            writing_body_to_client_;

        unsigned                                        restart_counter_;
        const boost::posix_time::time_duration          orgin_time_out_;
        boost::asio::deadline_timer                     read_timer_;
        boost::asio::deadline_timer                     write_timer_;

        static const http::filter                       connection_headers_to_be_removed_;
        static const unsigned                           max_restarts_ = 3;
    };

    ////////////////////////////////////////
    // response implementation
    template <class Connection, std::size_t BodyBufferSize>
    response<Connection, BodyBufferSize>::~response()
    {
        connection_->trait().event_proxy_response_destroyed(*connection_, *this);

        if ( proxy_socket_ )
            connector_.dismiss_connection(proxy_socket_);

        connection_->response_completed(*this);
    }

    template <class Connection, std::size_t BodyBufferSize>
    void response<Connection, BodyBufferSize>::start()
    {
        connection_->trait().event_proxy_response_started(*connection_, *this);

        connector_.async_get_proxy_connection<typename Connection::socket_t>(
            request_->host(), request_->port(),
            boost::bind(&response::handle_orgin_connect, this->shared_from_this(), _1, _2));

        // while waiting for the response, the request to the orgin server can be assembled
        outbuffers_ = filtered_header(*request_);
    }

    template < class Connection, std::size_t BodyBufferSize >
    const char* response<Connection, BodyBufferSize>::name() const
    {
        return "proxy::response";
    }

    template <class Connection, std::size_t BodyBufferSize>
    bool response<Connection, BodyBufferSize>::restart()
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

        connector_.async_get_proxy_connection<typename Connection::socket_t>(
            request_->host(), request_->port(),
            boost::bind(&response::handle_orgin_connect, this->shared_from_this(), _1, _2));

        return true;
    }

    template <class Connection, std::size_t BodyBufferSize>
    void response<Connection, BodyBufferSize>::disable_restart()
    {
        restart_counter_ = max_restarts_;
    }

    template <class Connection, std::size_t BodyBufferSize>
    void response<Connection, BodyBufferSize>::handle_orgin_connect(
        typename Connection::socket_t*      orgin_socket,
        const boost::system::error_code&    error)
    {
        connection_->trait().event_proxy_orgin_connected(*connection_, *this, orgin_socket, error);

        error_guard fail(*connection_, *this, http::http_bad_gateway);

        if ( !error && orgin_socket )
        {   
            proxy_socket_ = orgin_socket;

            server::async_write_with_to(
                *proxy_socket_,
                outbuffers_, 
                boost::bind(
                    &response::request_written,
                    this->shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred),
                write_timer_,
                orgin_time_out_);

            fail.dismiss();
        } 
    }
        
    template <class Connection, std::size_t BodyBufferSize>
    void response<Connection, BodyBufferSize>::request_written(
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
        else if ( restart() )
        {
            fail.dismiss();
        }
        else if ( error == make_error_code(server::time_out) )
        {
            fail.set_error_code(http::http_gateway_timeout);
        }
    }

    template <class Connection, std::size_t BodyBufferSize>
    void response<Connection, BodyBufferSize>::handle_read_from_orgin(
        const boost::system::error_code&    error,
        std::size_t                         bytes_transferred)
    {
        error_guard fail(*connection_, *this, http::http_bad_gateway);
        reading_body_from_orgin_ = false;

        if ( error )
        {
            connection_->trait().log_error(*this, "response::handle_read_from_orgin", error, bytes_transferred);

            if ( restart() )
                fail.dismiss();

            if ( error == make_error_code(server::time_out) )
                fail.set_error_code(http::http_gateway_timeout);

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
    void response<Connection, BodyBufferSize>::response_header_written(
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
    void response<Connection, BodyBufferSize>::handle_body_written(
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
    void response<Connection, BodyBufferSize>::issue_read(const std::pair<char*, std::size_t>& buffer)
    {
        issue_read(boost::asio::buffer(buffer.first, buffer.second));
    }

    template <class Connection, std::size_t BodyBufferSize>
    void response<Connection, BodyBufferSize>::issue_read(const boost::asio::mutable_buffers_1& buffer)
    {
        if ( !reading_body_from_orgin_ && buffer_size(buffer) != 0 )
        {
            reading_body_from_orgin_ = true;

            async_read_some_with_to(
                *proxy_socket_,
                buffer,
                boost::bind(&response::handle_read_from_orgin,
                        this->shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred),
                read_timer_,
                orgin_time_out_
            );
        }
    }

    template <class Connection, std::size_t BodyBufferSize>
    void response<Connection, BodyBufferSize>::orgin_timeout(const boost::system::error_code& error)
    {
        if ( !error && proxy_socket_ )
        {
            connection_->trait().log_error(*this, "response::orgin_timeout");

            // close connection to orgin, pass the connection back to the connector
            connector_.dismiss_connection(proxy_socket_);
            proxy_socket_ = 0;
        }
    }

    template <class Connection, std::size_t BodyBufferSize>
    void response<Connection, BodyBufferSize>::issue_write(const boost::asio::const_buffers_1& buffer)
    {
        if ( !writing_body_to_client_ && buffer_size(buffer) != 0 )
        {
            writing_body_to_client_ = true;
            connection_->async_write_some(
                buffer,
                boost::bind(&response::handle_body_written,
                        this->shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred),
                *this
            );
        }
    }

    template <class Connection, std::size_t BodyBufferSize>
    void response<Connection, BodyBufferSize>::forward_header()
    {
        // filter all connection headers
        outbuffers_ = filtered_header(response_header_from_proxy_);
        writing_body_to_client_ = true;

        connection_->async_write(
                outbuffers_, 
                boost::bind(
                    &response::response_header_written,
                    this->shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred),
                *this);
    }

    template <class Connection, std::size_t BodyBufferSize>
    template <class Header>
    std::vector<tools::substring> response<Connection, BodyBufferSize>::filtered_header(const Header& header)
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
    const http::filter response<Connection, BodyBufferSize>::connection_headers_to_be_removed_("connection, keep-alive");
} // namespace server

#endif 
