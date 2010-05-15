// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/test_response.h"

namespace server {
namespace test {

    response::response(const boost::shared_ptr<const request_header>& header, const boost::shared_ptr<response_chain_link>& next, const std::string& answer)
        : async_response(next)
        , answer_(answer.begin(), answer.end())
        , this_()
    {
    }

    void response::simulate_incomming_data()
    {
        start_send_data(buffer_full, &answer_[0], answer_.size());
        this_.reset();
    }

    async_response::state response::start_io()
    {
        this_ = shared_from_this(this);
        return io_pending;
    }

    async_response::state response::end_send_data(const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        return done;
    }

    boost::weak_ptr<response> create_response(const boost::shared_ptr<const request_header>& header, const boost::weak_ptr<response_chain_link>& next, const std::string& answer)
    {
        boost::shared_ptr<response> new_response(new response(header, boost::shared_ptr<response_chain_link>(next), answer));

        new_response->start();

        return boost::weak_ptr<response>(new_response);
    }


} // namespace test
} // namespace server
