// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/unittest++.h"
#include "server/test_response.h"
#include "server/test_request_texts.h"
#include "server/request.h"
#include <boost/asio/buffer.hpp>

namespace {
    class test_connection : public server::response_chain_link
    {
    public:
        std::string output() const
        {
            return std::string(output_.begin(), output_.end());
        }

        bool process()
        {
            const bool result = last_size_ != 0;

            if ( last_size_ )
            {
                handle_async_write(make_error_code(boost::system::errc::success), last_size_, shared_from_this());
                last_size_ = 0;
            }

            return result;
        }
    private:
        void issue_async_write(const boost::asio::const_buffer& buffer, const boost::shared_ptr<response_chain_link>&)
        {
            const char* const ptr = boost::asio::buffer_cast<const char*>(buffer);
            output_.insert(output_.begin(), ptr, ptr + buffer_size(buffer));

            last_size_ = buffer_size(buffer);
        }

        void handle_async_write(const boost::system::error_code& error, std::size_t bytes_transferred, const boost::shared_ptr<response_chain_link>& me)
        {
        }

        std::vector<char>   output_;
        std::size_t         last_size_;
    };

}

/** 
 * @test test that the 
 */
TEST(simply_receiving_a_hello)
{
    boost::shared_ptr<server::response_chain_link>  connection(new test_connection);

    // a simulated response to an empty header
    boost::shared_ptr<server::request_header>       empty_header(new server::request_header);

    boost::weak_ptr<server::test::response>         response1 = server::test::create_response(empty_header, connection, "hello");
    boost::weak_ptr<server::test::response>         response2 = server::test::create_response(empty_header, response1, " wie");
    boost::weak_ptr<server::test::response>         response3 = server::test::create_response(empty_header, response2, " gehts?");

    // then we simulate that the IO is fullfied
    boost::shared_ptr<server::test::response>(response1)->simulate_incomming_data();
    boost::shared_ptr<server::test::response>(response2)->simulate_incomming_data();
    boost::shared_ptr<server::test::response>(response3)->simulate_incomming_data();

    while ( boost::static_pointer_cast<test_connection>(connection)->process() )
        ;

    CHECK_EQUAL(0, response1.use_count());
    CHECK_EQUAL(0, response2.use_count());
    CHECK_EQUAL(0, response3.use_count());

    CHECK_EQUAL("hello wie gehts?", boost::static_pointer_cast<test_connection>(connection)->output());

}

