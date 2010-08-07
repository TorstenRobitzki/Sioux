// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/unittest++.h"
#include "server/proxy_connector.h"
#include "server/error_code.h"
#include "server/test_resolver.h"
#include "server/test_tools.h"
#include "server/test_socket.h"
#include "tools/iterators.h"
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/foreach.hpp>
#include <set>

using namespace server::test;

typedef server::test::socket<const char*> socket_t;
typedef server::ip_proxy_connector<socket_t> ip_proxy_connector;

namespace {
    template <class Socket = socket_t>
    struct connect_handler
    {
        connect_handler()
            : called(false)
            , connection()
            , error()
        {
        }

        void handle_connect(Socket* s, const boost::system::error_code& e)
        {
            assert(!called);
            called      = true;
            connection  = s ? *s : Socket();
            con_ptr     = s;
            error       = e;
        }

        bool                        called;
        Socket                      connection;
        Socket*                     con_ptr;
        boost::system::error_code   error;
    };
}

/**
 * @test 
 * The proxy connector should prefer using already connected connections over establishing new
 * connections.
 */
TEST(use_established_proxy_connections)
{
    const boost::asio::ip::tcp::endpoint            addr(boost::asio::ip::address::from_string("127.0.0.1"), 80);
    const server::proxy_configuration               config(
         server::proxy_configurator()
         .max_idle_time(boost::posix_time::seconds(2)));
    boost::asio::io_service                         queue;

    boost::shared_ptr<server::proxy_connector_base> proxy(new ip_proxy_connector(queue, config, addr));
    connect_handler<>                handler1;

    proxy->async_get_proxy_connection<socket_t>(
        tools::substring(), 0, 
        boost::bind(&connect_handler<>::handle_connect, boost::ref(handler1), _1, _2));

    run(queue);

    CHECK(handler1.called);
    CHECK(handler1.connection != socket_t());
    CHECK(handler1.con_ptr != 0);
    CHECK(!handler1.error);
    CHECK(handler1.connection.connected().first);
    CHECK_EQUAL(addr, handler1.connection.connected().second);

    proxy->dismiss_connection(handler1.con_ptr);

    connect_handler<>               handler2;
    proxy->async_get_proxy_connection<socket_t>(
        tools::substring(), 0, 
        boost::bind(&connect_handler<>::handle_connect, boost::ref(handler2), _1, _2));

    run(queue);

    CHECK(handler1.connection != handler2.connection);
    CHECK(handler2.called);
    CHECK(handler2.con_ptr != 0);
    CHECK(!handler2.error);
    CHECK(handler2.connection.connected().first);
    CHECK_EQUAL(addr, handler2.connection.connected().second);

    proxy->release_connection(handler2.con_ptr, http::response_header("HTTP/1.1 200 OK\r\n\r\n"));

    connect_handler<>               handler3;
    proxy->async_get_proxy_connection<socket_t>(
        tools::substring(), 0, 
        boost::bind(&connect_handler<>::handle_connect, boost::ref(handler3), _1, _2));

    run(queue);

    CHECK(handler3.connection == handler3.connection);
    CHECK(handler3.called);
    CHECK(handler3.con_ptr != 0);
    CHECK(!handler3.error);
    CHECK(handler3.connection.connected().first);
    CHECK_EQUAL(addr, handler3.connection.connected().second);

    proxy->release_connection(handler3.con_ptr, http::response_header("HTTP/1.1 200 OK\r\nconnection:foobar\r\n\r\n"));

    connect_handler<>               handler4;
    proxy->async_get_proxy_connection<socket_t>(
        tools::substring(), 0, 
        boost::bind(&connect_handler<>::handle_connect, boost::ref(handler4), _1, _2));

    run(queue);

    CHECK(handler4.connection == handler3.connection);
    CHECK(handler4.called);
    CHECK(handler4.con_ptr != 0);
    CHECK(!handler4.error);
    CHECK(handler4.connection.connected().first);
    CHECK_EQUAL(addr, handler4.connection.connected().second);

    proxy->release_connection(handler4.con_ptr, http::response_header("HTTP/1.1 200 OK\r\nconnection:close\r\n\r\n"));

    connect_handler<>               handler5;
    proxy->async_get_proxy_connection<socket_t>(
        tools::substring(), 0, 
        boost::bind(&connect_handler<>::handle_connect, boost::ref(handler5), _1, _2));

    run(queue);

    CHECK(handler5.connection != handler4.connection);
    CHECK(handler5.called);
    CHECK(handler5.con_ptr != 0);
    CHECK(!handler5.error);
    CHECK(handler5.connection.connected().first);
    CHECK_EQUAL(addr, handler5.connection.connected().second);

    proxy->release_connection(handler5.con_ptr, http::response_header("HTTP/1.1 200 OK\r\n\r\n"));

    // after waiting the idle timeout, a new connection have to be created
    wait(boost::posix_time::seconds(3));

    // this will forth the timeout handler to be executed
    run(queue);

    connect_handler<>               handler6;
    proxy->async_get_proxy_connection<socket_t>(
        tools::substring(), 0, 
        boost::bind(&connect_handler<>::handle_connect, boost::ref(handler6), _1, _2));

    run(queue);

    CHECK(handler5.connection != handler6.connection);
    CHECK(handler6.called);
    CHECK(handler6.con_ptr != 0);
    CHECK(!handler6.error);
    CHECK(handler6.connection.connected().first);
    CHECK_EQUAL(addr, handler6.connection.connected().second);

    proxy->release_connection(handler6.con_ptr, http::response_header("HTTP/1.1 200 OK\r\n\r\n"));

    // after waiting the idle timeout, a new connection have to be created, by not running the queue,
    // the call to async_get_proxy_connection() will find an idle connection and the idle time out will 
    // _not_ find the idle connection
    wait(boost::posix_time::seconds(3));

    connect_handler<>               handler7;
    proxy->async_get_proxy_connection<socket_t>(
        tools::substring(), 0, 
        boost::bind(&connect_handler<>::handle_connect, boost::ref(handler7), _1, _2));

    run(queue);

    CHECK(handler7.connection == handler6.connection);
    CHECK(handler7.called);
    CHECK(handler7.con_ptr != 0);
    CHECK(!handler7.error);
    CHECK(handler7.connection.connected().first);
    CHECK_EQUAL(addr, handler7.connection.connected().second);

    proxy->release_connection(handler7.con_ptr, http::response_header("HTTP/1.1 200 OK\r\n\r\n"));
}

/**
 * @test this test requests new connections, without handing them back and simulates the connected right
 *       after the request was made
 */
TEST(proxy_connection_limit)
{
    const boost::asio::ip::tcp::endpoint            addr(boost::asio::ip::address::from_string("192.168.1.1"), 88);
    const server::proxy_configuration               config(
         server::proxy_configurator()
         .max_connections(5));
    boost::asio::io_service                         queue;

    boost::shared_ptr<server::proxy_connector_base> proxy(
        new ip_proxy_connector(queue, config, addr));

    std::vector<socket_t*>   sockets;

    for ( unsigned connect_attempt = 0; connect_attempt != 10; ++connect_attempt )
    {
        connect_handler<>               handler;

        if ( connect_attempt < 5 )
        {
            proxy->async_get_proxy_connection<socket_t>(
                tools::substring(), 0, 
                boost::bind(&connect_handler<>::handle_connect, boost::ref(handler), _1, _2));

            run(queue);
            
            CHECK(handler.called);
            CHECK(handler.con_ptr != 0);
            CHECK(!handler.error);
            CHECK(handler.connection.connected().first);
            CHECK_EQUAL(addr, handler.connection.connected().second);

            sockets.push_back(handler.con_ptr);
        }
        else
        {
            CHECK_THROW(
                proxy->async_get_proxy_connection<socket_t>(
                    tools::substring(), 0, 
                    boost::bind(&connect_handler<>::handle_connect, boost::ref(handler), _1, _2)),
                server::proxy_connection_limit_reached);

            CHECK(!handler.called);
        }
    }

    // all addresses must be distinct
    std::set<socket_t*> sorted(sockets.begin(), sockets.end());
    CHECK_EQUAL(sockets.size(), sorted.size());
   
    for ( std::vector<socket_t*>::const_iterator i = sockets.begin(); i != sockets.end(); ++i)
        proxy->dismiss_connection(*i);
}

/**
 * @test in a second test, first the connects will be requested and later, the connections are simulated
 */
TEST(proxy_connection_limit2)
{
    const boost::asio::ip::tcp::endpoint            addr(boost::asio::ip::address::from_string("192.168.1.1"), 88);
    const server::proxy_configuration               config(
         server::proxy_configurator()
         .max_connections(5));
    boost::asio::io_service                         queue;

    boost::shared_ptr<server::proxy_connector_base> proxy(
        new ip_proxy_connector(queue, config, addr));

    std::vector<connect_handler<> > handler(5);

    for ( unsigned connect_attempt = 0; connect_attempt != 10; ++connect_attempt )
    {
        if ( connect_attempt < 5 )
        {
            proxy->async_get_proxy_connection<socket_t>(
                tools::substring(), 0, 
                boost::bind(&connect_handler<>::handle_connect, boost::ref(handler[connect_attempt]), _1, _2));
        }
        else
        {
            connect_handler<> handler;

            CHECK_THROW(
                proxy->async_get_proxy_connection<socket_t>(
                    tools::substring(), 0, 
                    boost::bind(&connect_handler<>::handle_connect, boost::ref(handler), _1, _2)),
                server::proxy_connection_limit_reached);

            CHECK(!handler.called);
        }
    }
    
    run(queue);

    for ( std::vector<connect_handler<> >::const_iterator i = handler.begin(); i != handler.end(); ++i )
    {
        CHECK(i->called);
        CHECK(i->con_ptr != 0);
        CHECK(!i->error);
        CHECK(i->connection.connected().first);
        CHECK_EQUAL(addr, i->connection.connected().second);

        proxy->dismiss_connection(i->con_ptr);
    }
}

TEST(proxy_connection_error)
{
    const boost::asio::ip::tcp::endpoint            addr(boost::asio::ip::address::from_string("192.168.1.1"), 88);
    const server::proxy_configuration               config;
    boost::asio::io_service                         queue;

    // use a socket type, that will simulate a connect error
    typedef server::test::socket<const char*, socket_behaviour<connect_error<error_on_connect> > > socket_t;
    typedef server::ip_proxy_connector<socket_t> ip_proxy_connector;

    boost::shared_ptr<server::proxy_connector_base> proxy(
        new ip_proxy_connector(queue, config, addr));

    connect_handler<socket_t>   handler;
    proxy->async_get_proxy_connection<socket_t>(
        tools::substring(), 0, 
        boost::bind(&connect_handler<socket_t>::handle_connect, boost::ref(handler), _1, _2));

    run(queue);

    CHECK(handler.called);
    CHECK(handler.con_ptr == 0);
    CHECK(handler.error);
}

TEST(proxy_connection_timeout)
{
    const boost::asio::ip::tcp::endpoint            addr(boost::asio::ip::address::from_string("192.168.1.1"), 88);
    const server::proxy_configuration               config(
         server::proxy_configurator()
         .connect_timeout(boost::posix_time::seconds(5)));
    boost::asio::io_service                         queue;

    // use a socket type, that will answer the connect request
    typedef server::test::socket<const char*, socket_behaviour<connect_error<do_not_respond> > > socket_t;
    typedef server::ip_proxy_connector<socket_t> ip_proxy_connector;

    boost::shared_ptr<server::proxy_connector_base> proxy(
        new ip_proxy_connector(queue, config, addr));

    connect_handler<socket_t>   handler;
    proxy->async_get_proxy_connection<socket_t>(
        tools::substring(), 0, 
        boost::bind(&connect_handler<socket_t>::handle_connect, boost::ref(handler), _1, _2));

    const boost::posix_time::ptime start = boost::posix_time::microsec_clock::universal_time();
    run(queue);
    const boost::posix_time::time_duration lasting = boost::posix_time::microsec_clock::universal_time() - start;

    CHECK_CLOSE(boost::posix_time::seconds(5), lasting, boost::posix_time::seconds(1));
    CHECK(handler.called);
    CHECK(handler.con_ptr == 0);
    CHECK_EQUAL(make_error_code(server::time_out), handler.error );
}

namespace {

    struct connector_client
    {
        connector_client(server::proxy_connector_base* p, unsigned n)
            : proxy_(p)
            , remaining_connects_(n)
        {
        }

        void start()
        {
            proxy_->async_get_proxy_connection<socket_t>(
                tools::substring(), 0, 
                boost::bind(&connector_client::handle_connect, this, _1, _2));
        }

        void handle_connect(socket_t* s, const boost::system::error_code& e)
        {
            assert(s);
            assert(!e);

            if ( remaining_connects_ % 1 == 1 )
            {
                static const http::response_header ok200("HTTP/1.1 200 OK\r\n\r\n");
                proxy_->release_connection(s, ok200);
            }
            else
            {
                proxy_->dismiss_connection(s);
            }

            --remaining_connects_;

            if ( remaining_connects_ )
                start();
        }

        server::proxy_connector_base*   proxy_;
        unsigned                        remaining_connects_;
    };
}

TEST(proxy_connection_stress)
{
    const server::proxy_configuration               config(
         server::proxy_configurator()
         .connect_timeout(boost::posix_time::seconds(5)));
    boost::asio::io_service                         queue;
    const boost::asio::ip::tcp::endpoint            addr(boost::asio::ip::address::from_string("192.168.1.1"), 88);

    boost::shared_ptr<server::proxy_connector_base> proxy(
        new ip_proxy_connector(queue, config, addr));

    std::vector<connector_client>                   clients(5, connector_client(&*proxy, 200000));

    BOOST_FOREACH(connector_client & c, clients)
    {
        c.start();
    }

    run(queue, 5);

    BOOST_FOREACH(connector_client & c, clients)
    {
        CHECK(c.remaining_connects_ == 0);
    }
}
