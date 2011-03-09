// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/transfer_buffer.h"
#include "http/test_request_texts.h"
#include "http/response.h"
#include "unittest++/UnitTest++.h"
#include <boost/asio/buffers_iterator.hpp>

using namespace http::test;

TEST(chunked_encoding_transfer)
{
    http::response_header header(chunked_response_example);
    CHECK_EQUAL(http::message::ok, header.state());

    server::transfer_buffer<1000> buffer;
    buffer.start(header);

    buffer.data_read(header.unparsed_buffer().second);
    CHECK(buffer.transmission_done());
}

TEST(content_length_transfer) 
{
    http::response_header header(
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 20\r\n\r\n"
        "01234567890123456789");
    CHECK_EQUAL(http::message::ok, header.state());

    server::transfer_buffer<1000> buffer;
    buffer.start(header);

    std::pair<char*, std::size_t> body = header.unparsed_buffer();

    buffer.data_read(header.unparsed_buffer().second);
    CHECK(buffer.transmission_done());
}

TEST(eof_transfer)
{
    http::response_header header(
        "HTTP/1.1 200 OK\r\n\r\n"
        "01234567890123456789");
    CHECK_EQUAL(http::message::ok, header.state());

    server::transfer_buffer<1000> buffer;
    buffer.start(header);

    std::pair<char*, std::size_t> body = header.unparsed_buffer();

    buffer.data_read(header.unparsed_buffer().second);
    CHECK(!buffer.transmission_done());
    buffer.data_written(0);
    CHECK(buffer.transmission_done());
}

