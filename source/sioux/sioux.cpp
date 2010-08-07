// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/connection.h"
#include "server/traits.h"
#include "server/proxy_connector.h"
#include <iostream>
#include <exception>
#include <boost/asio.hpp>

using namespace boost::asio;

class traits_t : public server::connection_traits<response_factory>
{
public:
    traits_t()
        : config_(server::proxy_configurator()
            .connect_timeout(boost::posix_time::seconds(5))
            .max_connections(10)
            .max_idle_time(boost::posix_time::seconds(30))
        , proxy_(new ip_proxy_connector<ip::tcp::socket>())
    {
    }

    server::proxy_configuration                     config_;
    boost::shared_ptr<server::proxy_connector_base> proxy_;
};

template <class Connection>
class response_factory 
{
    template <class Trait>
    static boost::shared_ptr<async_response> create_response(
        const boost::shared_ptr<Connection>&                    connection,
        const boost::shared_ptr<const http::request_header>&    header,
              Trait&                                            trait)
    {
        const boost::shared_ptr<response<Connection> > new_response(new response<Connection>(connection, header, "Hello"));
        return boost::shared_ptr<async_response>(new_response);
    }
};

typedef server::connection<traits_t, ip::tcp::socket>   connection_t;

class acceptator
{
public:
    acceptator(io_service& s)
        : end_point_()
        , acceptor_(s, ip::tcp::endpoint(ip::tcp::v6(), 80))
        , queue_(s)
        , connection_()
    {
        issue_accept();
    }

private:
    void issue_accept()
    {
        connection_.reset(new connection_t(boost::ref(queue_), trait_));
        acceptor_.async_accept(connection_->socket(), end_point_, boost::bind(&acceptator::handler, this, boost::asio::placeholders::error));
    }

    void handler(const boost::system::error_code& error)
    {
        if ( !error )
        {
            std::clog<< "new connection: " << end_point_ << std::endl;

            connection_->start();
            issue_accept();
        }
        else
        {
            std::cerr << "handler: " << error << std::endl;
        }
    }

    ip::tcp::endpoint               end_point_;
    ip::tcp::acceptor               acceptor_;
    io_service&                     queue_;
    boost::shared_ptr<connection_t> connection_;
    traits_t                        trait_;
};

int main()
{
    std::cout << "this is Sioux 0.1" << std::endl;

    try
    {
        traits_t    trait;
        io_service  io_queue;

        acceptator acceptor(io_queue);
        io_queue.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "error: " << e.what() << std::endl;
    }
}