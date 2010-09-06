// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/unittest++.h"
#include "server/proxy_response.h"
#include "server/connection.h"
#include "http/request.h"
#include "http/response.h"

#include "http/test_request_texts.h"
#include "server/test_socket.h"
#include "server/test_traits.h"
#include "server/test_proxy.h"
#include "server/test_tools.h"
 
using namespace server::test;
using namespace http::test;

namespace {
    template <std::size_t BufferSize = 1024>
    struct sized_proxy_response_factory
    {
        template <class Connection>
        struct proxy_response_factory
        {
            template <class Trait>
            static boost::shared_ptr<server::async_response> create_response(
                const boost::shared_ptr<Connection>&                    connection,
                const boost::shared_ptr<const http::request_header>&    header,
                      Trait&                                            trait)
            {
                return boost::shared_ptr<server::async_response>(
                    new server::proxy_response<Connection>(connection, header, trait.proxy()));
            }
        };
    };
}

template <std::size_t BufferSize>
static std::vector<char> simulate_sized_proxy(
    const boost::shared_ptr<proxy_connector>&               proxy,
          server::test::socket<const char*>&                output)
{
    typedef server::test::socket<const char*>           socket_t;
    typedef traits<socket_t, sized_proxy_response_factory<BufferSize>::proxy_response_factory>    trait_t;
    typedef server::connection<trait_t, socket_t>       connection_t;

    boost::asio::io_service&                            queue = proxy->get_io_service();
    trait_t                                             trait(*proxy);

    boost::shared_ptr<connection_t>                     connection(new connection_t(output, trait));
    connection->start();

    boost::weak_ptr<connection_t>                       con_ref(connection);
    connection.reset();

    run(queue); 

    trait.reset_responses();
    run(queue); 
    if ( !con_ref.expired() )
        throw std::runtime_error("expected connection to be expired.");

    return output.bin_output();
}

// response send over the client connection
static std::string simulate_proxy(
    const boost::shared_ptr<proxy_connector>&               proxy,
    const tools::substring&                                 request)
{
    server::test::socket<const char*> output(proxy->get_io_service(), request.begin(), request.end());

    const std::vector<char> bin = simulate_sized_proxy<1024>(proxy, output);

    return std::string(bin.begin(), bin.end());
}

static std::string through_proxy(const boost::shared_ptr<const http::request_header>& r, const std::string& orgin_response)
{
    boost::asio::io_service queue;
    boost::shared_ptr<proxy_connector> proxy(new proxy_connector(queue, orgin_response));
    simulate_proxy(proxy, r->text());

    return proxy->received();
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

/**
 * @test correct host and port are connected
 */
TEST(correct_host_and_port_connected)
{
    http::request_header request(
        "GET / HTTP/1.1\r\n"
        "host: 127.0.0.1:8080\r\n\r\n");

    boost::asio::io_service queue;
    boost::shared_ptr<proxy_connector> proxy(new proxy_connector(queue, cached_response_apache));
    simulate_proxy(proxy, request.text());
    
    CHECK_EQUAL("127.0.0.1", proxy->connected_orgin_server().first);
    CHECK_EQUAL(8080u, proxy->connected_orgin_server().second);
}

/**
 * @test that a valid error response will be generated, when connecting the orgin is not possible
 */
TEST(responde_when_no_connection_to_orgin_possible)
{
    http::request_header request(
        "GET / HTTP/1.1\r\n"
        "host: 127.0.0.1:8080\r\n\r\n");

    boost::asio::io_service queue;
    boost::shared_ptr<proxy_connector> proxy(new proxy_connector(queue, proxy_connector::connection_not_possible));
    const std::string response_text = simulate_proxy(proxy, request.text());
    http::response_header response(response_text.c_str());
    
    CHECK_EQUAL(http::message::ok, response.state());
    CHECK_EQUAL(http::http_internal_server_error, response.code());

}

/**
 * @test the proxy config asynchronously responses with an error
 */
TEST(error_while_connecting_the_orgin_server)
{
    http::request_header request(
        "GET / HTTP/1.1\r\n"
        "host: 127.0.0.1:8080\r\n\r\n");

    boost::asio::io_service queue;
    boost::shared_ptr<proxy_connector> proxy(new proxy_connector(queue, proxy_connector::error_while_connecting));
    const std::string response_text = simulate_proxy(proxy, request.text());
    http::response_header response(response_text.c_str());
    
    CHECK_EQUAL(http::message::ok, response.state());
    CHECK_EQUAL(http::http_bad_gateway, response.code());
}

/**
 * @test while writing the request header to the proxy,
 */
TEST(error_while_writing_header_to_orgin_server)
{
    http::request_header request(
        "GET / HTTP/1.1\r\n"
        "host: 127.0.0.1:8080\r\n\r\n");

    boost::asio::io_service queue;
    server::test::socket<const char*> orgin_connection(
        queue, 
        begin(cached_response_apache), end(cached_response_apache),
        make_error_code(boost::system::errc::success), 10000,
        boost::asio::error::connection_aborted, 15);

    boost::shared_ptr<proxy_connector> proxy(new proxy_connector(orgin_connection));
    const std::string response_text = simulate_proxy(proxy, request.text());

    http::response_header response(response_text.c_str());
    
    CHECK_EQUAL(http::message::ok, response.state());
    CHECK_EQUAL(http::http_bad_gateway, response.code());
}

/**
 * @test while reading the response header from the proxy, an error occured
 */
TEST(error_while_reading_header_from_orgin_server)
{
    http::request_header request(
        "GET / HTTP/1.1\r\n"
        "host: 127.0.0.1:8080\r\n\r\n");

    boost::asio::io_service queue;
    server::test::socket<const char*> orgin_connection(
        queue, 
        begin(cached_response_apache), end(cached_response_apache),
        boost::asio::error::connection_aborted, 15,
        make_error_code(boost::system::errc::success), 10000);
 
    // use 5 connections, because the proxy response will reconnect to the orgin server
    proxy_connector::socket_list_t orgin_connections(5, orgin_connection);
    boost::shared_ptr<proxy_connector> proxy(new proxy_connector(orgin_connections));
    const std::string response_text = simulate_proxy(proxy, request.text());

    http::response_header response(response_text.c_str());
    
    CHECK_EQUAL(http::message::ok, response.state());
    CHECK_EQUAL(http::http_bad_gateway, response.code());
}

/**
 * @test connection headers must not be forwarded, but must be replaced with our own
 *
 * Simulate a response from the orgin server, with a connection header.
 */
TEST(remove_connection_headers_from_orgin_response)
{
    http::request_header request(
        "GET / HTTP/1.1\r\n"
        "host: 127.0.0.1:8080\r\n\r\n");

    boost::asio::io_service queue;
    boost::shared_ptr<proxy_connector> proxy(new proxy_connector(queue, cached_response_apache));
    
    const std::string response_text = simulate_proxy(proxy, request.text());
    http::response_header response(response_text.c_str());
    
    CHECK_EQUAL(http::message::ok, response.state());
    CHECK_EQUAL(http::http_not_modified, response.code());
    CHECK(response.find_header("connection") == 0);
    CHECK(response.find_header("Keep-Alive") == 0);

}

TEST(big_random_chunked_body)
{
    const char response_text[] =
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked\r\n\r\n";

    boost::minstd_rand      random;

    std::vector<char> proxy_response = random_chunk(random, random_body(random, 1 * 1024 * 1024), 2048);
    proxy_response.insert(proxy_response.begin(), begin(response_text), end(response_text));

    boost::asio::io_service queue;

    // test with an proxy buffer of 1024 bytes
    { 
        server::test::socket<const char*>   client_connection(queue, begin(get_local_root_firefox), end(get_local_root_firefox), 
                                random, 5, 40);
        server::test::socket<const char*>   proxy_connection(queue, &proxy_response[0], &proxy_response[0] + proxy_response.size(),
                                random, 1, 2048);

        boost::shared_ptr<proxy_connector> proxy(new proxy_connector(proxy_connection));

        const std::vector<char> client_received = simulate_sized_proxy<1024>(proxy, client_connection);

        CHECK_EQUAL(proxy_response.size(), client_received.size());
        CHECK(compare_buffers(proxy_response, client_received, std::cerr));
    }

    // test with a proxy buffer of 200 bytes
    { 
        server::test::socket<const char*>   client_connection(queue, begin(get_local_root_firefox), end(get_local_root_firefox), 
                                random, 5, 40);
        server::test::socket<const char*>   proxy_connection(queue, &proxy_response[0], &proxy_response[0] + proxy_response.size(),
                                random, 1, 2048);

        boost::shared_ptr<proxy_connector> proxy(new proxy_connector(proxy_connection));

        const std::vector<char> client_received = simulate_sized_proxy<200>(proxy, client_connection);

        CHECK_EQUAL(proxy_response.size(), client_received.size());
        CHECK(compare_buffers(proxy_response, client_received, std::cerr));
    }

    // test with a proxy buffer of 20 kbytes
    { 
        server::test::socket<const char*>   client_connection(queue, begin(get_local_root_firefox), end(get_local_root_firefox), 
                                random, 5, 40);
        server::test::socket<const char*>   proxy_connection(queue, &proxy_response[0], &proxy_response[0] + proxy_response.size(),
                                random, 1, 2048);

        boost::shared_ptr<proxy_connector> proxy(new proxy_connector(proxy_connection));

        const std::vector<char> client_received = simulate_sized_proxy<20*1024>(proxy, client_connection);

        CHECK_EQUAL(proxy_response.size(), client_received.size());
        CHECK(compare_buffers(proxy_response, client_received, std::cerr));
    }
}

TEST(content_length_proxy_request)
{    
    const char response_text[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 10000\r\n\r\n";

    boost::minstd_rand      random;

    std::vector<char> proxy_response = random_body(random, 10000);
    proxy_response.insert(proxy_response.begin(), begin(response_text), end(response_text));

    boost::asio::io_service queue;

    server::test::socket<const char*>   client_connection(queue, begin(get_local_root_firefox), end(get_local_root_firefox), 
                            random, 5, 40);
    server::test::socket<const char*>   proxy_connection(queue, &proxy_response[0], &proxy_response[0] + proxy_response.size(),
                            random, 1, 2048);

    boost::shared_ptr<proxy_connector> proxy(new proxy_connector(proxy_connection));

    const std::vector<char> client_received = simulate_sized_proxy<1024>(proxy, client_connection);

    CHECK_EQUAL(proxy_response.size(), client_received.size());
    CHECK(compare_buffers(proxy_response, client_received, std::cerr));
}

TEST(close_connection_length_proxy_request)
{    
    const char response_text[] =
        "HTTP/1.1 200 OK\r\n\r\n";

    boost::minstd_rand      random;

    std::vector<char> proxy_response = random_body(random, 10000);
    proxy_response.insert(proxy_response.begin(), begin(response_text), end(response_text));

    boost::asio::io_service queue;

    server::test::socket<const char*>   client_connection(queue, begin(get_local_root_firefox), end(get_local_root_firefox), 
                            random, 5, 40);
    server::test::socket<const char*>   proxy_connection(queue, &proxy_response[0], &proxy_response[0] + proxy_response.size(),
                            random, 1, 2048);

    boost::shared_ptr<proxy_connector> proxy(new proxy_connector(proxy_connection));

    const std::vector<char> client_received = simulate_sized_proxy<1024>(proxy, client_connection);

    CHECK_EQUAL(proxy_response.size(), client_received.size());
    CHECK(compare_buffers(proxy_response, client_received, std::cerr));
}

TEST(request_an_other_connection_when_the_first_was_falty)
{
    boost::asio::io_service         queue;
    proxy_connector::socket_list_t  orgin_connections;

    orgin_connections.push_back(
        server::test::socket<const char*>(
            queue, 
            &chunked_response_example[0], &chunked_response_example[0],
            make_error_code(boost::asio::error::network_reset),0,
            make_error_code(boost::asio::error::network_reset),10000));

    orgin_connections.push_back(
        server::test::socket<const char*>(
            queue, begin(chunked_response_example), end(chunked_response_example)));

    boost::shared_ptr<proxy_connector> connector(new proxy_connector(orgin_connections));

    CHECK_EQUAL(chunked_response_example,
        simulate_proxy(connector, tools::substring(begin(simple_get_11), end(simple_get_11))));
} 

TEST(timeout_while_reading_from_orgin)
{
    CHECK(false);
}

TEST(timeout_while_writing_to_orgin)
{
    CHECK(false);
}