// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/unittest++.h"
#include "server/test_socket.h"
#include "server/test_tools.h"
#include "http/test_request_texts.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/read.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/bind.hpp>

using namespace server::test;
using namespace http::test;

namespace {
    struct io_completed
    {
        void handle_data(const boost::system::error_code& e, std::size_t b)
        {
            error = e;
            bytes_transferred = b;
        }

        boost::system::error_code   error;
        size_t                      bytes_transferred;
    };
}

TEST(read_timeout_test)
{
    io_completed result;

    boost::asio::io_service queue;
    server::test::socket<const char*> sock(queue, begin(simple_get_11), end(simple_get_11), 5, boost::posix_time::seconds(1));
    char b[sizeof simple_get_11];

    const boost::posix_time::ptime t1 = boost::posix_time::microsec_clock::universal_time();

    sock.async_read_some(boost::asio::buffer(b), boost::bind<void>(&io_completed::handle_data, &result, _1, _2));
    run(queue);

    CHECK_CLOSE(boost::posix_time::seconds(1), boost::posix_time::microsec_clock::universal_time() - t1, boost::posix_time::millisec(100));
    CHECK_EQUAL(5u, result.bytes_transferred);
    CHECK(!result.error);
}

TEST(write_timeout_test)
{
    io_completed result;

    boost::asio::io_service queue;
    server::test::socket<const char*> sock(queue, begin(simple_get_11), end(simple_get_11), 5, boost::posix_time::seconds(1));

    const boost::posix_time::ptime t1 = boost::posix_time::microsec_clock::universal_time();

    sock.async_write_some(boost::asio::buffer(simple_get_11, 5), boost::bind<void>(&io_completed::handle_data, &result, _1, _2));
    run(queue);

    CHECK_CLOSE(boost::posix_time::seconds(1), boost::posix_time::microsec_clock::universal_time() - t1, boost::posix_time::millisec(100));
    CHECK_EQUAL(5u, result.bytes_transferred);
    CHECK(!result.error);
}

TEST(chancel_read_write)
{
    io_completed result_read;
    io_completed result_write;

    boost::asio::io_service queue;
    server::test::socket<const char*> sock(queue, begin(simple_get_11), end(simple_get_11), 5, boost::posix_time::seconds(1));
    char b[sizeof simple_get_11];

    sock.async_read_some(boost::asio::buffer(b), boost::bind<void>(&io_completed::handle_data, &result_read, _1, _2));
    sock.async_write_some(boost::asio::buffer(simple_get_11, 5), boost::bind<void>(&io_completed::handle_data, &result_write, _1, _2));
    sock.close();

    run(queue);

    CHECK_EQUAL(0u, result_read.bytes_transferred);
    CHECK_EQUAL(0u, result_write.bytes_transferred);
    CHECK_EQUAL(make_error_code(boost::asio::error::operation_aborted), result_read.error);
    CHECK_EQUAL(make_error_code(boost::asio::error::operation_aborted), result_write.error);
}