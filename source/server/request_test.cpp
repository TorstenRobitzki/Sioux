// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/unittest++.h"
#include "server/request.h"
#include "server/test_request_texts.h"
#include <algorithm>

using namespace server::test;

namespace {
    // returns true, if the header consumed all input
    template <unsigned S>
    bool feed_to_request(const char(&text)[S], server::request_header& header)
    {
        for ( unsigned remaining = S-1; remaining; )
        {
            const std::pair<char*, std::size_t> mem         = header.read_buffer();
            const std::size_t                   copy_size   = std::min(mem.second, remaining);

            std::copy(&text[S-1-remaining], &text[S-1-remaining+copy_size], mem.first);
            remaining -= copy_size;

            if ( header.parse(copy_size) )
                return remaining == 0;
        }

        return true;
    }

    template <unsigned S>
    server::request_header feed_to_request(const char(&text)[S])
    {
        server::request_header  request;
        feed_to_request(text, request);

        return request;
    }
}

TEST(parse_methods)
{
    const server::request_header options = feed_to_request("OPTIONS / HTTP/1.1\r\n\r\n");
    CHECK_EQUAL(server::request_header::ok, options.state());
    CHECK_EQUAL(http::http_options, options.method());

    const server::request_header get     = feed_to_request("GET / HTTP/1.1\r\n\r\n");
    CHECK_EQUAL(server::request_header::ok, get.state());
    CHECK_EQUAL(http::http_get, get.method());

    const server::request_header head    = feed_to_request("HEAD / HTTP/1.1\r\n\r\n");
    CHECK_EQUAL(server::request_header::ok, head.state());
    CHECK_EQUAL(http::http_head, head.method());

    const server::request_header post    = feed_to_request("POST / HTTP/1.1\r\n\r\n");
    CHECK_EQUAL(server::request_header::ok, post.state());
    CHECK_EQUAL(http::http_post, post.method());

    const server::request_header put     = feed_to_request("PUT / HTTP/1.1\r\n\r\n");
    CHECK_EQUAL(server::request_header::ok, put.state());
    CHECK_EQUAL(http::http_put, put.method());

    const server::request_header delete_ = feed_to_request("DELETE / HTTP/1.1\r\n\r\n");
    CHECK_EQUAL(server::request_header::ok, delete_.state());
    CHECK_EQUAL(http::http_delete, delete_.method());

    const server::request_header trace   = feed_to_request("TRACE / HTTP/1.1\r\n\r\n");
    CHECK_EQUAL(server::request_header::ok, trace.state());
    CHECK_EQUAL(http::http_trace, trace.method());

    const server::request_header connect = feed_to_request("CONNECT / HTTP/1.1\r\n\r\n");
    CHECK_EQUAL(server::request_header::ok, connect.state());
    CHECK_EQUAL(http::http_connect, connect.method());
}

TEST(parse_broken_methods)
{
    const server::request_header options = feed_to_request("OPTIONs / HTTP/1.1\r\n\r\n");
    CHECK_EQUAL(server::request_header::syntax_error, options.state());

    const server::request_header get     = feed_to_request(" GET / HTTP/1.1\r\n\r\n");
    CHECK_EQUAL(server::request_header::syntax_error, get.state());

    const server::request_header head    = feed_to_request("H_EAD / HTTP/1.1\r\n\r\n");
    CHECK_EQUAL(server::request_header::syntax_error, head.state());

    const server::request_header post    = feed_to_request("P OST / HTTP/1.1\r\n\r\n");
    CHECK_EQUAL(server::request_header::syntax_error, post.state());

    const server::request_header put     = feed_to_request("pUT / HTTP/1.1\r\n\r\n");
    CHECK_EQUAL(server::request_header::syntax_error, put.state());

    const server::request_header delete_ = feed_to_request("DELET / HTTP/1.1\r\n\r\n");
    CHECK_EQUAL(server::request_header::syntax_error, delete_.state());

    const server::request_header trace   = feed_to_request("RACE / HTTP/1.1\r\n\r\n");
    CHECK_EQUAL(server::request_header::syntax_error, trace.state());

    const server::request_header connect = feed_to_request("CONNECTGET / HTTP/1.1\r\n\r\n");
    CHECK_EQUAL(server::request_header::syntax_error, connect.state());
}
 
TEST(simple_request)
{
    server::request_header  request;
    CHECK_EQUAL(server::request_header::parsing, request.state());

    CHECK(feed_to_request(simple_get_11, request));

    CHECK_EQUAL(server::request_header::ok, request.state());
    CHECK_EQUAL(1u, request.major_version());
    CHECK_EQUAL(1u, request.minor_version());
    CHECK(request.uri() == "/");
    CHECK_EQUAL(http::http_get, request.method());
}

TEST(parse_versions)
{
    const server::request_header v12_21 = feed_to_request("OPTIONS / http/12.21\r\n\r\n");
    CHECK_EQUAL(server::request_header::ok, v12_21.state());
    CHECK_EQUAL(12u, v12_21.major_version());
    CHECK_EQUAL(21u, v12_21.minor_version());

    const server::request_header v01_01 = feed_to_request("OPTIONS / Http/01.01\r\n\r\n");
    CHECK_EQUAL(server::request_header::ok, v01_01.state());
    CHECK_EQUAL(1u, v01_01.major_version());
    CHECK_EQUAL(1u, v01_01.minor_version());
}

TEST(check_options_available)
{
    const server::request_header header = feed_to_request(
        "OPTIONS / http/12.21\r\n"
        "Connection : close  \r\n"
        "accept:text/plain,text/html\r\n"
        "Accept-Encoding : compress, gzip\r\n"
        "\r\n\r\n");

    CHECK_EQUAL(server::request_header::ok, header.state());
    CHECK(header.option_available("connection", "close"));
    CHECK(header.option_available("accept", "text/plain"));
    CHECK(header.option_available("accept", "text/html"));
    CHECK(header.option_available("accept-encoding", "compress"));
    CHECK(header.option_available("accept-encoding", "gzip"));


}