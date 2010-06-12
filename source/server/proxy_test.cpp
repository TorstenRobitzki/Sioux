// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/unittest++.h"
#include "server/proxy.h"
#include "server/connection.h"
#include "http/request.h"

#include "http/test_request_texts.h"
#include "server/test_socket.h"
#include "server/test_traits.h"
#include "server/test_proxy.h"

using namespace server::test;
using namespace http::test;

// returns the request send to the orgin server and the response send over the client connection
static std::pair<std::string, std::string> simulate_proxy(
    const boost::shared_ptr<const http::request_header>&    request, 
    const std::string&                                      orgin_response)
{
    typedef server::test::socket<const char*>       socket_t;
    typedef server::connection<traits<>, socket_t>  connection_t;
    typedef server::proxy_response<connection_t>    response_t;

    socket_t                                                output;
    proxy_config                                            config(orgin_response);

    boost::shared_ptr<connection_t>                         connection(new connection_t(output, traits<>()));
    connection->start();

    boost::shared_ptr<response_t>                           response( new response_t(connection, request, config));
    response->start();

    boost::weak_ptr<connection_t>                           con_ref(connection);
    boost::weak_ptr<response_t>                             resp_ref(response);
    connection.reset();
    response.reset();

    for ( ; config.process() || output.process(); )
        ;

    if ( !con_ref.expired() )
        throw std::runtime_error("expected connection to be expired.");

    if ( !resp_ref.expired() )
        throw std::runtime_error("expected response to be expired.");

    return std::make_pair(config.received(), output.output());
}

static std::string through_proxy(const boost::shared_ptr<const http::request_header>& r, const std::string& orgin_response)
{
    return simulate_proxy(r, orgin_response).first;
}

/**
 * @brief check, that connection headers are removed from the original request
 */
TEST(check_removed_headers)
{
    const boost::shared_ptr<const http::request_header> opera(new http::request_header(get_local_root_opera));
    const boost::shared_ptr<const http::request_header> firefox(new http::request_header(get_local_root_firefox));
    const boost::shared_ptr<const http::request_header> internet_explorer(new http::request_header(get_local_root_internet_explorer));
    const boost::shared_ptr<const http::request_header> generic(new http::request_header(
        "GET / HTTP/1.1\r\n"
        "bla: blub\r\n"
        "Connection:bla, \r\n"
        "\tfoo, bar\r\n"
        "foo: bar\r\n"
        "host:127.0.0.1\r\n"
        "\r\n"));

    CHECK_EQUAL(http::request_header::ok, opera->state());
    CHECK_EQUAL(http::request_header::ok, firefox->state());
    CHECK_EQUAL(http::request_header::ok, internet_explorer->state());
    CHECK_EQUAL(http::request_header::ok, generic->state());

    CHECK(opera->find_header("Connection"));
    CHECK(firefox->find_header("Connection"));
    CHECK(firefox->find_header("Keep-Alive"));
    CHECK(internet_explorer->find_header("Connection"));
    CHECK(generic->find_header("bla"));
    CHECK(generic->find_header("foo"));
    CHECK(generic->find_header("connection"));

    // now, tunnel each of them through a proxy
    {
        std::string response_text = through_proxy(opera, cached_response_apache);
        http::request_header opera(response_text.c_str());
        CHECK_EQUAL(http::request_header::ok, opera.state());
        CHECK(!opera.find_header("Connection"));

        CHECK(opera.find_header("User-Agent"));
        CHECK(opera.find_header("Host"));
        CHECK(opera.find_header("Accept"));
        CHECK(opera.find_header("Accept-Language"));
        CHECK(opera.find_header("Accept-Charset"));
        CHECK(opera.find_header("Accept-Encoding"));
    }

    {
        std::string response_text = through_proxy(firefox, cached_response_apache);
        http::request_header firefox(response_text.c_str());
        CHECK_EQUAL(http::request_header::ok, firefox.state());
        CHECK(!firefox.find_header("Connection"));
        CHECK(!firefox.find_header("Keep-Alive"));

        CHECK(firefox.find_header("Host"));
        CHECK(firefox.find_header("User-Agent"));
        CHECK(firefox.find_header("Accept"));
        CHECK(firefox.find_header("Accept-Language"));
        CHECK(firefox.find_header("Accept-Encoding"));
        CHECK(firefox.find_header("Accept-Charset"));
    }

    {
        std::string response_text = through_proxy(internet_explorer, cached_response_apache);
        http::request_header internet_explorer(response_text.c_str());
        CHECK_EQUAL(http::request_header::ok, internet_explorer.state());
        CHECK(!internet_explorer.find_header("Connection"));
    }

    {
        std::string response_text = through_proxy(generic, cached_response_apache);
        http::request_header generic(response_text.c_str());
        CHECK_EQUAL(http::request_header::ok, generic.state());
        CHECK(!generic.find_header("bla"));
        CHECK(!generic.find_header("bar"));
        CHECK(!generic.find_header("foo"));
        CHECK(!generic.find_header("connection"));

        CHECK(generic.find_header("host"));
    }

}

TEST(unsupported_http_version)
{
    const boost::shared_ptr<const http::request_header> http_10_request(new http::request_header(
        "GET / HTTP/1.0\r\n"
        "host:127.0.0.1\r\n"
        "\r\n"));

    CHECK_EQUAL(http::request_header::ok, http_10_request->state());

//    const std::string response = simulate_proxy(http_10_request, "").second;

}