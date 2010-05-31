// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/connection.h"
#include "server/test_traits.h"
#include <iostream>
#include <exception>
#include <boost/asio.hpp>

using namespace boost::asio;
typedef server::test::traits<ip::tcp::socket>           traits_t;
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