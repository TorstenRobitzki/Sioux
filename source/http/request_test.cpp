// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/unittest++.h"
#include "http/request.h"
#include "http/test_request_texts.h"
#include "http/filter.h"
#include <algorithm>
#include <iterator>

using namespace http::test;

namespace {
    // returns true, if the header consumed all input
    template <unsigned S>
    bool feed_to_request(const char(&text)[S], http::request_header& header)
    {
        for ( unsigned remaining = S-1; remaining; )
        {
            const std::pair<char*, std::size_t> mem         = header.read_buffer();
            const std::size_t                   copy_size   = std::min(static_cast<unsigned>(mem.second), remaining);

            std::copy(&text[S-1-remaining], &text[S-1-remaining+copy_size], mem.first);
            remaining -= copy_size;

            if ( header.parse(copy_size) )
                return remaining == 0;
        }

        return true;
    }

    template <unsigned S>
    http::request_header feed_to_request(const char(&text)[S])
    {
    	http::request_header  request;
        feed_to_request(text, request);

        return request;
    }
}

TEST(parse_methods)
{
    const http::request_header options("OPTIONS / HTTP/1.1\r\nhost:\r\n\r\n");
    CHECK_EQUAL(http::request_header::ok, options.state());
    CHECK_EQUAL(http::http_options, options.method());

    const http::request_header get("GET / HTTP/1.1\r\nhost:\r\n\r\n");
    CHECK_EQUAL(http::request_header::ok, get.state());
    CHECK_EQUAL(http::http_get, get.method());

    const http::request_header head("HEAD / HTTP/1.1\r\nhost:\r\n\r\n");
    CHECK_EQUAL(http::request_header::ok, head.state());
    CHECK_EQUAL(http::http_head, head.method());

    const http::request_header post("POST / HTTP/1.1\r\nhost:\r\n\r\n");
    CHECK_EQUAL(http::request_header::ok, post.state());
    CHECK_EQUAL(http::http_post, post.method());

    const http::request_header put("PUT / HTTP/1.1\r\nhost:\r\n\r\n");
    CHECK_EQUAL(http::request_header::ok, put.state());
    CHECK_EQUAL(http::http_put, put.method());

    const http::request_header delete_("DELETE / HTTP/1.1\r\nhost:\r\n\r\n");
    CHECK_EQUAL(http::request_header::ok, delete_.state());
    CHECK_EQUAL(http::http_delete, delete_.method());

    const http::request_header trace("TRACE / HTTP/1.1\r\nhost:\r\n\r\n");
    CHECK_EQUAL(http::request_header::ok, trace.state());
    CHECK_EQUAL(http::http_trace, trace.method());

    const http::request_header connect("CONNECT / HTTP/1.1\r\nhost:\r\n\r\n");
    CHECK_EQUAL(http::request_header::ok, connect.state());
    CHECK_EQUAL(http::http_connect, connect.method());
}

TEST(parse_broken_methods)
{
    const http::request_header options("OPTIONs / HTTP/1.1\r\nhost:\r\n\r\n");
    CHECK_EQUAL(http::request_header::syntax_error, options.state());

    const http::request_header get(" GET / HTTP/1.1\r\nhost:\r\n\r\n");
    CHECK_EQUAL(http::request_header::syntax_error, get.state());

    const http::request_header head("H_EAD / HTTP/1.1\r\nhost:\r\n\r\n");
    CHECK_EQUAL(http::request_header::syntax_error, head.state());

    const http::request_header post("P OST / HTTP/1.1\r\nhost:\r\n\r\n");
    CHECK_EQUAL(http::request_header::syntax_error, post.state());

    const http::request_header put("pUT / HTTP/1.1\r\nhost:\r\n\r\n");
    CHECK_EQUAL(http::request_header::syntax_error, put.state());

    const http::request_header delete_("DELET / HTTP/1.1\r\nhost:\r\n\r\n");
    CHECK_EQUAL(http::request_header::syntax_error, delete_.state());

    const http::request_header trace("RACE / HTTP/1.1\r\nhost:\r\n\r\n");
    CHECK_EQUAL(http::request_header::syntax_error, trace.state());

    const http::request_header connect("CONNECTGET / HTTP/1.1\r\nhost:\r\n\r\n");
    CHECK_EQUAL(http::request_header::syntax_error, connect.state());
}
 
TEST(simple_request)
{
    http::request_header  request;
    CHECK_EQUAL(http::request_header::parsing, request.state());

    CHECK(feed_to_request(simple_get_11, request));

    CHECK_EQUAL(http::request_header::ok, request.state());
    CHECK_EQUAL(1u, request.major_version());
    CHECK_EQUAL(1u, request.minor_version());
    CHECK(request.uri() == "/");
    CHECK_EQUAL(http::http_get, request.method());
}

TEST(parse_versions)
{
    const http::request_header v12_21("OPTIONS / http/12.21\r\nhost:\r\n\r\n");
    CHECK_EQUAL(http::request_header::ok, v12_21.state());
    CHECK_EQUAL(12u, v12_21.major_version());
    CHECK_EQUAL(21u, v12_21.minor_version());

    const http::request_header v01_01("OPTIONS / Http/01.01\r\nhost:\r\n\r\n");
    CHECK_EQUAL(http::request_header::ok, v01_01.state());
    CHECK_EQUAL(1u, v01_01.major_version());
    CHECK_EQUAL(1u, v01_01.minor_version());
}

TEST(check_options_available)
{
    const http::request_header header(
        "OPTIONS / http/12.21\r\n"
        "host:\r\n"
        "Connection : close  \r\n"
        "accept:text/plain,\t\r\n"
        " text/html\r\n"
        "Accept-Encoding : compress, gzip\r\n"
        "\r\n");

    CHECK_EQUAL(http::request_header::ok, header.state());
    CHECK(header.option_available("connection", "close"));
    CHECK(header.option_available("accept", "text/plain"));
    CHECK(header.option_available("accept", "text/html"));
    CHECK(header.option_available("accept-encoding", "compress"));
    CHECK(header.option_available("accept-encoding", "gzip"));
}

TEST(single_arguement_ctor)
{
    const http::request_header header(
        "OPTIONS / http/12.21\r\n"
        "host:\r\n"
        "Connection : close  \r\n"
        "accept:text/plain,text/html\r\n"
        "Accept-Encoding : compress, gzip\r\n"
        "\r\n");

    CHECK_EQUAL(http::request_header::ok, header.state());
    CHECK(header.option_available("connection", "close"));
    CHECK(header.option_available("accept", "text/plain"));
    CHECK(header.option_available("accept", "text/html"));
    CHECK(header.option_available("accept-encoding", "compress"));
    CHECK(header.option_available("accept-encoding", "gzip"));
}

TEST(test_text)
{
    const http::request_header header(simple_get_11);
    CHECK_EQUAL(simple_get_11, header.text());
}

static std::string to_header(const std::vector<tools::substring>& text)
{
    std::ostringstream buffer;
    for ( std::vector<tools::substring>::const_iterator begin = text.begin(), end = text.end(); begin != end; ++begin )
        buffer.write(begin->begin(), begin->size());

    return buffer.str();
}

TEST(filter_header_test)
{
    const http::request_header header(simple_get_11);
    const http::filter           without("Accept-Charset,Referer");

    CHECK(header.find_header("Host"));
    CHECK(header.find_header("User-Agent"));
    CHECK(header.find_header("Accept-Encoding"));
    CHECK(header.find_header("Accept-Charset"));
    CHECK(header.find_header("Cache-Control"));
    CHECK(header.find_header("Accept-Language"));
    CHECK(header.find_header("Referer"));

    const http::request_header filtered(to_header(header.filtered_request_text(without)).c_str());
    CHECK_EQUAL(http::request_header::ok, filtered.state());
    CHECK(filtered.find_header("Host"));
    CHECK(filtered.find_header("User-Agent"));
    CHECK(filtered.find_header("Accept-Encoding"));
    CHECK(!filtered.find_header("Accept-Charset"));
    CHECK(filtered.find_header("Cache-Control"));
    CHECK(filtered.find_header("Accept-Language"));
    CHECK(!filtered.find_header("Referer"));

    CHECK_EQUAL(header.uri(), filtered.uri());
    CHECK_EQUAL(header.milli_version(), filtered.milli_version());
    CHECK_EQUAL(header.method(), filtered.method());

    // unfiltered
    const http::request_header same(to_header(header.filtered_request_text(http::filter())).c_str());
    CHECK_EQUAL(http::request_header::ok, same.state());
    CHECK_EQUAL(1u, header.filtered_request_text(http::filter()).size());
    CHECK_EQUAL(header.text(), same.text());

    const http::request_header other(to_header(filtered.filtered_request_text(http::filter("bla, cache-control"))).c_str());
    CHECK_EQUAL(http::request_header::ok, other.state());
    CHECK(other.find_header("Host"));
    CHECK(other.find_header("User-Agent"));
    CHECK(other.find_header("Accept-Encoding"));
    CHECK(!other.find_header("Cache-Control"));
    CHECK(other.find_header("Accept-Language"));
}

TEST(check_header_value_test)
{
    const http::request_header request(
        "GET / HTTP/1.1\r\n"
		"Host: google.de\r\n"
        "FooBar :\trababer\r\n"
        "\tboobar\r\n"
        " foo   \t \r\n"
        "Nase:\r\n"
        "\r\n");

    CHECK_EQUAL(http::request_header::ok, request.state());
    const http::header* header = request.find_header("foobar");

    CHECK(header != 0);
    CHECK(header != 0 && header->value() == "rababer\r\n\tboobar\r\n foo");
}

TEST(check_host_and_port)
{
    { // no port given, default is 80        
        const http::request_header request(
            "GET / HTTP/1.1\r\n"
            "host :foobar\r\n"
            "\r\n");

        CHECK_EQUAL(http::message::ok, request.state());
        CHECK_EQUAL("foobar", request.host());
        CHECK_EQUAL(80u, request.port());
    }

    { // 3.2.2 http URL " If the port is _empty_ or not given, port 80 is assumed. 
        const http::request_header request(
            "GET / HTTP/1.1\r\n"
            "host :foobar.com:\r\n"
            "\r\n");

        CHECK_EQUAL(http::message::ok, request.state());
        CHECK_EQUAL("foobar.com", request.host());
        CHECK_EQUAL(80u, request.port());
    }

    { // port given
        const http::request_header request(
            "GET / HTTP/1.1\r\n"
            "host :foobar.com:90\r\n"
            "\r\n");

        CHECK_EQUAL(http::message::ok, request.state());
        CHECK_EQUAL("foobar.com", request.host());
        CHECK_EQUAL(90u, request.port());
    }

    { // empty host and empty port
        const http::request_header request(
            "GET / HTTP/1.1\r\n"
            "host ::\r\n"
            "\r\n");

        CHECK_EQUAL(http::message::ok, request.state());
        CHECK_EQUAL("", request.host());
        CHECK_EQUAL(80u, request.port());
    }
}

/**
 * @test checks, that the function unparsed_buffer() returns the correct values
 */
TEST(check_unparsed_buffer)
{
    http::request_header request(
        "GET / HTTP/1.1\r\n"
        "host:\r\n"
        "\r\n"
        "abs");

    CHECK_EQUAL(http::message::ok, request.state());
    CHECK_EQUAL('a', *request.unparsed_buffer().first);
    CHECK_EQUAL(3u, request.unparsed_buffer().second);
}


