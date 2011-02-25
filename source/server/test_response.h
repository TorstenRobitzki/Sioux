// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_TEST_RESPONSE_H
#define SIOUX_SOURCE_SERVER_TEST_RESPONSE_H

#include "server/response.h"
#include "tools/substring.h"
#include "http/http.h"
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/system/error_code.hpp>
#include <string>

namespace http {
    class request_header;
}

namespace server{

    
namespace test {

    enum response_type {
        /// reponse is automaticaly added
        auto_response,
        /// incomming data, that is needed to response must be simulated by calling simulate_incomming_data()
        manuel_response 
    };

    /**
     * @brief a async_reponse implementation, that that responses with a given text
     */
    template <class Connection>
    class response : public async_response, public boost::enable_shared_from_this<response<Connection> >
    {
    public:
        response(const boost::shared_ptr<Connection>& connection, const boost::shared_ptr<const http::request_header>& /*header*/, const std::string& answer)
            : connection_(connection)
            , answer_(answer)
            , response_type_(auto_response)
            , error_code_()
            , error_(false)
        {
        }

        response(const boost::shared_ptr<Connection>& connection, const boost::shared_ptr<const http::request_header>& /*header*/, const std::string& answer, response_type rt)
            : connection_(connection)
            , answer_(answer)
            , response_type_(rt)
            , error_code_()
            , error_(false)
        {
        }

        response(const boost::shared_ptr<Connection>& connection, const boost::shared_ptr<const http::request_header>& /*header*/, http::http_error_code answer, response_type rt)
            : connection_(connection)
            , answer_()
            , response_type_(rt)
            , error_code_(answer)
            , error_(true)
        {
        }

        void start()
        {
            self_ = this->shared_from_this();

            if ( response_type_ == auto_response )
                simulate_incomming_data();
        }

        void simulate_incomming_data()
        {
            self_.reset();

            if ( error_ )
            {
                connection_->response_not_possible(*this, error_code_);
            }
            else
            {
                connection_->async_write(
                    boost::asio::buffer(answer_), 
                    boost::bind(&response::handler, 
                            this->shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred),
                    *this);
            }
        }

    private:
        void handler(const boost::system::error_code& /*error*/, std::size_t /*bytes_transferred*/)
        {
            connection_->response_completed(*this); 
        }

        const boost::shared_ptr<Connection> connection_;
        const std::string                   answer_;
        boost::shared_ptr<response>         self_;
        response_type                       response_type_;
        http::http_error_code               error_code_;
        bool                                error_;
    };


} // namespace test

} // namespace server


#endif

