// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SOURCE_SIOUX_SERVER_ERROR_H
#define SOURCE_SIOUX_SERVER_ERROR_H

#include "http/http.h"
#include "server/response.h"
#include "tools/asstring.h"
#include <boost/enable_shared_from_this.hpp>
#include <boost/utility.hpp>
#include <boost/asio/buffer.hpp>

namespace server {

    template <class Connection>
    class error_response : public async_response, 
                           public boost::enable_shared_from_this<error_response<Connection> >, 
                           private boost::noncopyable
    {
    public:
        explicit error_response(const boost::shared_ptr<Connection>& con, http::http_error_code ec);

        void start();
    private:
        void handle_written(
            const boost::system::error_code&    error,
            std::size_t                         bytes_transferred);

        std::string                     buffer_;
        boost::shared_ptr<Connection>   connection_;
    };


    template <class Connection>
    error_response<Connection>::error_response(const boost::shared_ptr<Connection>& con, http::http_error_code ec)
        : buffer_("HTTP/1.1 " + tools::as_string(static_cast<int>(ec)) + " " + http::reason_phrase(ec) + "\r\nContent-Length:0\r\n\r\n")
     , connection_(con)
    {
    }

    template <class Connection>
    void error_response<Connection>::start()
    {
        connection_->async_write(
            boost::asio::buffer(buffer_),
            boost::bind(
                &error_response::handle_written, 
                shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred),
            *this);
    }

    template <class Connection>
    void error_response<Connection>::handle_written(const boost::system::error_code&, std::size_t)
    {
        connection_->response_completed(*this);
    }

} // namespace server

#endif // include guard