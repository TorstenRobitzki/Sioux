// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/test_proxy.h"
#include <boost/bind.hpp>
#include <typeinfo>

namespace server
{
namespace test {

//////////////////////
// class proxy_config 
proxy_config::proxy_config(boost::asio::io_service& queue, const std::string& simulate_response)
 : io_service_(queue)
 , simulate_response_(simulate_response.begin(), simulate_response.end())
 , error_type_(no_error)
 , socket_(queue, &simulate_response_[0], &simulate_response_[0] + simulate_response_.size())
 , socket_in_use_(false) 
{
}

proxy_config::proxy_config(socket<const char*>& socket)
 : io_service_(socket.get_io_service())
 , simulate_response_()
 , error_type_(no_error)
 , socket_(socket)
 , socket_in_use_(false)
{
}

proxy_config::proxy_config(boost::asio::io_service& queue, error_type error)
 : io_service_(queue)
 , simulate_response_()
 , error_type_(error)
 , socket_(queue)
 , socket_in_use_(false)
{
}

proxy_config::~proxy_config()
{
    assert(!socket_in_use_);
}

std::string proxy_config::received() const
{
    return socket_.output();
}

std::pair<std::string, unsigned> proxy_config::connected_orgin_server() const
{
    return requested_orgin_;
}

boost::asio::io_service& proxy_config::get_io_service()
{
    return io_service_;
}
       
void proxy_config::call_cb(connect_callback* p)
{
    std::auto_ptr<connect_callback> del(p);

    if ( error_type_ == error_while_connecting )
    {
        p->connection_received(0, make_error_code(boost::asio::error::host_not_found));
    }
    else
    {
        socket_in_use_ = true;
        p->connection_received(&socket_, make_error_code(boost::system::errc::success));
    }
}

void proxy_config::async_get_proxy_connection(
    const tools::dynamic_type&          connection_type,
    const tools::substring&             orgin_host,
    unsigned                            orgin_port,
    std::auto_ptr<connect_callback>&    call_back)
{
    if ( error_type_ == connection_not_possible ) 
        throw proxy_error("connection_not_possible");

    requested_orgin_ = std::make_pair(std::string(orgin_host.begin(), orgin_host.end()), orgin_port);

    if ( connection_type != typeid (server::test::socket<const char*>) )
        throw std::runtime_error("test::proxy_config::async_get_proxy_connection: invalid type"); 

    io_service_.post(boost::bind(&proxy_config::call_cb, this, call_back.get()));
    call_back.release();
}

void proxy_config::release_connection(
    const tools::dynamic_type&          connection_type,
    void*                               connection,
    const http::response_header*        /* header */)
{
    if ( connection_type != typeid (server::test::socket<const char*>) )
        throw std::runtime_error("test::proxy_config::release_connection: invalid type"); 

    assert(connection == &socket_);
    socket_in_use_ = false;
}

} // namespace test
} // namespace server
