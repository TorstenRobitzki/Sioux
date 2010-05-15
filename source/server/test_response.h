// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_TEST_RESPONSE_H
#define SIOUX_SOURCE_SERVER_TEST_RESPONSE_H

#include "server/response.h"
#include "tools/substring.h"
#include <boost/shared_ptr.hpp>
#include <string>

namespace server{

    class request_header;

namespace test {

    class response : public async_response
    {
    public:
        response(const boost::shared_ptr<const request_header>& header, const boost::shared_ptr<response_chain_link>& next, const std::string& answer);

        void simulate_incomming_data();
    private:
        virtual state start_io();

        virtual state end_send_data(const boost::system::error_code& error, std::size_t bytes_transferred);

        const std::vector<char>                 answer_;
        boost::shared_ptr<response_chain_link>  this_;
    };

    boost::weak_ptr<response> create_response(const boost::shared_ptr<const request_header>& header, const boost::weak_ptr<response_chain_link>& next, const std::string& answer);

} // namespace test

} // namespace server


#endif

