// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/unittest++.h"
#include "server/proxy.h"
#include "server/request.h"
#include "server/connection.h"

#include "server/test_request_texts.h"
#include "server/test_socket.h"
#include "server/test_traits.h"
#include "server/test_proxy.h"

using namespace server::test;

// returns the request send to the orgin server and the response send over the client connection
static std::pair<std::string, std::string> simulate_proxy(const server::request_header& r, const std::string& orgin_response)
{
    typedef server::test::socket<const char*>       socket_t;
    typedef server::connection<traits<>, socket_t>  connection_t;
    typedef server::proxy_response<connection_t>    response_t;

    socket_t                                                output;
    proxy_config                                            config(orgin_response);
    const boost::shared_ptr<const server::request_header>   request(new server::request_header(r));

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

static std::string through_proxy(const server::request_header& r, const std::string& orgin_response)
{
    return simulate_proxy(r, orgin_response).first;
}

/**
 * @brief check, that connection headers are removed from the original request
 */
TEST(check_removed_headers)
{
    const server::request_header opera(get_local_root_opera);
    const server::request_header firefox(get_local_root_firefox);
    const server::request_header internet_explorer(get_local_root_internet_explorer);
    const server::request_header generic(
        "GET / HTTP/1.1\r\n"
        "bla: blub\r\n"
        "Connection:bla, foo, bar\r\n"
        "foo: bar\r\n"
        "host:127.0.0.1\r\n"
        "\r\n");

    CHECK_EQUAL(server::request_header::ok, opera.state());
    CHECK_EQUAL(server::request_header::ok, firefox.state());
    CHECK_EQUAL(server::request_header::ok, internet_explorer.state());
    CHECK_EQUAL(server::request_header::ok, generic.state());

    CHECK(opera.find_header("Connection"));
    CHECK(firefox.find_header("Connection"));
    CHECK(firefox.find_header("Keep-Alive"));
    CHECK(internet_explorer.find_header("Connection"));
    CHECK(generic.find_header("bla"));
    CHECK(generic.find_header("foo"));
    CHECK(generic.find_header("connection"));

    // now, tunnel each of them through a proxy
    {
        std::string response_text = through_proxy(opera, cached_response_apache);
        server::request_header opera(response_text.c_str());
        CHECK_EQUAL(server::request_header::ok, opera.state());
        CHECK(!opera.find_header("Connection"));
    }

    {
        std::string response_text = through_proxy(opera, cached_response_apache);
        server::request_header opera(response_text.c_str());
        CHECK_EQUAL(server::request_header::ok, opera.state());
        CHECK(!opera.find_header("Connection"));
    }

}