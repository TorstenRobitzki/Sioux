// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SOURCE_SIOUX_SERVER_ERROR_H
#define SOURCE_SIOUX_SERVER_ERROR_H

#include "http/http.h"
#include "server/response.h"
#include "tools/asstring.h"
#include <boost/asio/buffer.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/system/error_code.hpp>
#include <boost/utility.hpp>

namespace server
{

    /**
     * @brief response with the given http code and with an empty body
     */
    template <class Connection>
    class error_response : public async_response, 
                           public boost::enable_shared_from_this<error_response<Connection> >, 
                           private boost::noncopyable
    {
    public:
        explicit error_response(const boost::shared_ptr<Connection>& con, http::http_error_code ec);

        void start();

        const char* name() const;
    private:
        void handle_written(
            const boost::system::error_code&    error,
            std::size_t                         bytes_transferred);

        std::string                     buffer_;
        boost::shared_ptr<Connection>   connection_;
    };

    /**
     * @brief just calls connection::response_not_possible() with the error code given to the c'tor
     *
     * This class is intended to be used by factories called from an response_factory, that want to indicate an
     * error but want to used the response_factories error response function.
     */
    template < class Connection >
    class defered_error_response :  public async_response,
                                    private boost::noncopyable
    {
    public:
        explicit defered_error_response(const boost::shared_ptr<Connection>& con, http::http_error_code ec);

    private:
        void start();
        const char* name() const;

        const boost::shared_ptr< Connection >   connection_;
        const http::http_error_code             code_;
    };

    // implementation
    template < class Connection >
    error_response< Connection >::error_response( const boost::shared_ptr<Connection>& con, http::http_error_code ec )
        : buffer_( "HTTP/1.1 "
            + tools::as_string( static_cast< int >( ec ) ) + " "
            + http::reason_phrase( ec )
            + "\r\nContent-Length:0\r\n\r\n" )
        , connection_( con )
    {
    }

    template < class Connection >
    void error_response< Connection >::start()
    {
        connection_->async_write(
            boost::asio::buffer( buffer_ ),
            boost::bind(
                &error_response::handle_written, 
                this->shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred),
            *this);
    }

    template < class Connection >
    const char* error_response< Connection >::name() const
    {
            return "server::error_response";
    }

    template <class Connection>
    void error_response<Connection>::handle_written(const boost::system::error_code&, std::size_t s)
    {
        connection_->response_completed(*this);
    }

    template < class Connection >
    defered_error_response< Connection >::defered_error_response(
        const boost::shared_ptr< Connection >& con, http::http_error_code ec)
        : connection_( con )
        , code_( ec )
    {
    }

    template < class Connection >
    void defered_error_response< Connection >::start()
    {
        connection_->response_not_possible( *this, code_ );
    }

    template < class Connection >
    const char* defered_error_response< Connection >::name() const
    {
        return "server::defered_error_response";
    }

} // namespace server

#endif // include guard
