// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>
#include "server/connection.h"
#include "server/test_traits.h"
#include "server/test_tools.h"
#include "http/test_request_texts.h"
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace server::test;
using namespace http::test; 

BOOST_AUTO_TEST_CASE(read_simple_header)
{
    boost::asio::io_service     queue;
    traits<>::connection_type   socket(queue, begin(simple_get_11), end(simple_get_11), 5);
    traits<>                    trait;

    server::create_connection(socket, trait);

    queue.run(); 

    BOOST_CHECK_EQUAL(1u, trait.requests().size());
}

BOOST_AUTO_TEST_CASE(read_multiple_header)
{
    boost::asio::io_service     queue;
    traits<>::connection_type   socket(queue, begin(simple_get_11), end(simple_get_11), 400, 2000);
    traits<>                    traits;

    server::create_connection(socket, traits);

    queue.run();

    BOOST_CHECK_EQUAL(2000u, traits.requests().size());
}

BOOST_AUTO_TEST_CASE(read_big_buffer)
{
    std::vector<char>   input;
    for ( unsigned i = 0; i != 1000; ++i )
        input.insert(input.end(), begin(simple_get_11), end(simple_get_11));

    typedef traits<server::test::socket<std::vector<char>::const_iterator> > socket_t;
    boost::asio::io_service     queue;
    socket_t::connection_type   socket(queue, input.begin(), input.end());
    traits<>                    traits;

    server::create_connection(socket, traits);

    queue.run();

    BOOST_CHECK_EQUAL(1000u, traits.requests().size());
}

BOOST_AUTO_TEST_CASE(read_buffer_overflow)
{
    const char header[] = "Accept-Encoding: gzip\r\n";

    std::vector<char>   input(begin(request_without_end_line), end(request_without_end_line));
    for ( unsigned i = 0; i != 10000; ++i )
        input.insert(input.end(), begin(header), end(header));

    typedef traits<server::test::socket<std::vector<char>::const_iterator> > socket_t;
    boost::asio::io_service     queue;
    socket_t::connection_type   socket(queue, input.begin(), input.end());
    traits<>                    traits;

    server::create_connection(socket, traits);

    queue.run();

    BOOST_CHECK_EQUAL(1u, traits.requests().size());
    BOOST_CHECK_EQUAL(http::request_header::buffer_full, traits.requests().front()->state());
}

/**
 * @brief sender closed connection, if request was ok, a response should have been send
 */
BOOST_AUTO_TEST_CASE(close_after_sender_closed)
{
    boost::asio::io_service     queue;
    traits<>::connection_type   socket(queue, begin(simple_get_11), end(simple_get_11), 0);
    traits<>                    trait;

    boost::weak_ptr<server::connection<traits<>, traits<>::connection_type> > connection(server::create_connection(socket, trait));
    BOOST_CHECK(!connection.expired());

    queue.run();

    trait.reset_responses();
    BOOST_CHECK_EQUAL("Hello", socket.output());
    BOOST_CHECK(connection.expired());
}

/**
 * @test client requests a close via connection header
 */
BOOST_AUTO_TEST_CASE(closed_by_connection_header)
{
    boost::asio::io_service     queue;
    traits<>::connection_type   socket(queue, begin(simple_get_11_with_close_header), end(simple_get_11_with_close_header), 0);
    traits<>                    trait;

    boost::weak_ptr<server::connection<traits<>, traits<>::connection_type> > connection(server::create_connection(socket, trait));
    BOOST_CHECK(!connection.expired());

    queue.run();

    trait.reset_responses();
    BOOST_CHECK_EQUAL("Hello", socket.output());

    // no outstanding reference to the connection object, so no read is pending on the connection to the client
    BOOST_CHECK(connection.expired());
}

/**
 * @test tests that a maximum idle time is not exceeded and the connection is closed 
 */
BOOST_AUTO_TEST_CASE(closed_when_idle_time_exceeded)
{
	using server::test::read;

    boost::asio::io_service     queue;
    read_plan                   reads;
    reads << read(begin(simple_get_11), end(simple_get_11))
          << delay(boost::posix_time::seconds(60))
          << read(begin(simple_get_11), end(simple_get_11))
          << read("");

    traits<>::connection_type   socket(queue, reads);
    traits<>                    trait;

    boost::weak_ptr<server::connection<traits<>, traits<>::connection_type> > connection(server::create_connection(socket, trait));
    BOOST_CHECK(!connection.expired());

    timer time;
    queue.run();

    // 30sec is the default idle time
    server::connection_config config;
    BOOST_CHECK_GE(time.elapsed(), config.keep_alive_timeout() - boost::posix_time::seconds(1));
    BOOST_CHECK_LE(time.elapsed(), config.keep_alive_timeout() + boost::posix_time::seconds(1));

    trait.reset_responses();
    BOOST_CHECK_EQUAL("Hello", socket.output());

    // no outstanding reference to the connection object, so no read is pending on the connection to the client
    BOOST_CHECK(connection.expired());
}

/**
 * @test the connection should be forced to be closed, when the connection is closed by the client
 */
BOOST_AUTO_TEST_CASE(closed_by_client_disconnected)
{
    boost::asio::io_service     queue;
    traits<>::connection_type   socket(queue, begin(simple_get_11), begin(simple_get_11) + (sizeof simple_get_11 / 2), 0);
    traits<>                    trait;

    boost::weak_ptr<server::connection<traits<>, traits<>::connection_type> > connection(server::create_connection(socket, trait));
    BOOST_CHECK(!connection.expired());

    queue.run();

    trait.reset_responses();
    BOOST_CHECK_EQUAL("", socket.output());
    BOOST_CHECK(connection.expired());
}

/**
 * @test handle timeout while writing to a client 
 */
BOOST_AUTO_TEST_CASE(timeout_while_writing_to_client)
{
	using server::test::read;
	using server::test::write;

	boost::asio::io_service     queue;

    read_plan                   reads;
    reads << read(begin(simple_get_11), end(simple_get_11))
          << delay(boost::posix_time::seconds(60))
          << read("");

    write_plan                  writes;
    writes << write(2) << delay(boost::posix_time::seconds(60));

    traits<>::connection_type   socket(queue, reads, writes);
    traits<>                    trait;

    boost::weak_ptr<server::connection<traits<>, traits<>::connection_type> > connection(server::create_connection(socket, trait));
    BOOST_CHECK(!connection.expired());

    timer time;
    queue.run();

    // 3sec is the default timeout
    server::connection_config config;
    BOOST_CHECK_GE(time.elapsed(), config.timeout() - boost::posix_time::seconds(1));
    BOOST_CHECK_LE(time.elapsed(), config.timeout() + boost::posix_time::seconds(1));

    trait.reset_responses();
    BOOST_CHECK_EQUAL("He", socket.output());

    // no outstanding reference to the connection object, so no read is pending on the connection to the client
    BOOST_CHECK(connection.expired());
}

/**
 * @test handle timeout while reading from a client
 *
 * The client starts sending a request header, but stops in the middle of the request to send further data.
 */
BOOST_AUTO_TEST_CASE(timeout_while_reading_from_client)
{
	using server::test::read;

	boost::asio::io_service     queue;
    read_plan                   reads;
    reads << read(begin(simple_get_11), begin(simple_get_11) + (sizeof simple_get_11 / 2) )
          << delay(boost::posix_time::seconds(60))
          << read(begin(simple_get_11), end(simple_get_11))
          << read("");

    traits<>::connection_type   socket(queue, reads);
    traits<>                    trait;

    boost::weak_ptr<server::connection<traits<>, traits<>::connection_type> > connection(server::create_connection(socket, trait));
    BOOST_CHECK(!connection.expired());

    timer time;
    queue.run();

    // 3sec is the default timeout
    server::connection_config config;
    BOOST_CHECK_GE(time.elapsed(), config.timeout() - boost::posix_time::seconds(1));
    BOOST_CHECK_LE(time.elapsed(), config.timeout() + boost::posix_time::seconds(1));

    trait.reset_responses();
    BOOST_CHECK_EQUAL("", socket.output());

    // no outstanding reference to the connection object, so no read is pending on the connection to the client

    BOOST_CHECK(connection.expired());
}
