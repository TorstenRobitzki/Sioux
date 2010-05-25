// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_TEST_RESPONSE_H
#define SIOUX_SOURCE_SERVER_TEST_RESPONSE_H

#include "server/response.h"
#include "tools/substring.h"
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/system/error_code.hpp>
#include <string>

namespace server{

    class request_header;

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
        response(const boost::shared_ptr<Connection>& connection, const boost::shared_ptr<const server::request_header>& /*header*/, const std::string& answer)
            : connection_(connection)
            , answer_(answer)
            , response_type_(auto_response)
        {
        }

        response(const boost::shared_ptr<Connection>& connection, const boost::shared_ptr<const server::request_header>& /*header*/, const std::string& answer, response_type rt)
            : connection_(connection)
            , answer_(answer)
            , response_type_(rt)
        {
        }

        void start()
        {
            self_ = shared_from_this();
            connection_->response_started(self_);

            if ( response_type_ == auto_response )
                simulate_incomming_data();
        }

        void simulate_incomming_data()
        {
            connection_->async_write_some(
                boost::asio::buffer(answer_), 
                boost::bind(&response::handler, 
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred),
                *this);

            self_.reset();
        }

    private:
        void handler(const boost::system::error_code& /*error*/, std::size_t /*bytes_transferred*/)
        {
            connection_->response_completed(*this); 
        }

        boost::shared_ptr<async_response>   self_;
        const boost::shared_ptr<Connection> connection_;
        const std::string                   answer_;
        response_type                       response_type_;
    };

    template <class Connection>
    boost::weak_ptr<async_response> create_response(
        const boost::shared_ptr<Connection>&                    connection, 
        const boost::shared_ptr<const server::request_header>&  header, 
        const std::string&                                      answer,
        response_type                                           response_procedure)
    {
        const boost::shared_ptr<response<Connection> > respo(new response<Connection>(connection, header, answer, response_procedure));
        respo->start();

        return boost::weak_ptr<async_response>(respo);
    }

} // namespace test

} // namespace server


#endif

