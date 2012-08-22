// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "proxy/test_connector.h"
#include <boost/bind.hpp>
#include <typeinfo>

namespace proxy
{
namespace test {

//////////////////////
// class proxy_config 
connector::connector(boost::asio::io_service& queue, const std::string& simulate_response)
 : io_service_(queue)
 , simulate_response_(simulate_response.begin(), simulate_response.end())
 , error_type_(no_error)
 , sockets_( 1, server::test::socket< const char* >(
     queue, &simulate_response_[0], &simulate_response_[0] + simulate_response_.size() ) )
 , sockets_in_use_() 
{
}

connector::connector( server::test::socket< const char* >& socket )
 : io_service_( socket.get_io_service() )
 , simulate_response_()
 , error_type_(no_error)
 , sockets_( 1, socket )
 , sockets_in_use_()
{
}

connector::connector(socket_list_t& socket)
 : io_service_(socket.front().get_io_service())
 , simulate_response_()
 , error_type_(no_error)
 , sockets_(socket)
 , sockets_in_use_()
{
}

connector::connector( boost::asio::io_service& queue, error_type error)
 : io_service_( queue )
 , simulate_response_()
 , error_type_(error)
 , sockets_(1, server::test::socket< const char* >( queue ) )
 , sockets_in_use_()
{
}

connector::~connector()
{
    assert(sockets_in_use_.empty());
}

std::string connector::received() const
{
    std::string result;

    for ( socket_list_t::const_iterator i = sockets_.begin(); i != sockets_.end(); ++i )
        result += i->output();

    return result;
}

std::pair<std::string, unsigned> connector::connected_orgin_server() const
{
    return requested_orgin_;
}

boost::asio::io_service& connector::get_io_service()
{
    return io_service_;
}
       
void connector::call_cb(const boost::shared_ptr<connect_callback>& p)
{
    if ( error_type_ == error_while_connecting )
    {
        p->connection_received(0, make_error_code(boost::asio::error::host_not_found));
    }
    else
    {
        assert(!sockets_.empty());
        sockets_in_use_.push_back(sockets_.front());
        sockets_.pop_front();
        p->connection_received(&sockets_in_use_.front(), make_error_code(boost::system::errc::success));
    }
}

void connector::async_get_proxy_connection(
    const tools::dynamic_type&                  connection_type,
    const tools::substring&                     orgin_host,
    unsigned                                    orgin_port,
    const boost::shared_ptr<connect_callback>&  call_back)
{
    assert(!sockets_.empty());

    if ( error_type_ == connection_not_possible ) 
        throw proxy::error( "connection_not_possible" );

    requested_orgin_ = std::make_pair(std::string(orgin_host.begin(), orgin_host.end()), orgin_port);

    if ( connection_type != typeid (server::test::socket<const char*>) )
        throw std::runtime_error("test::proxy_config::async_get_proxy_connection: invalid type"); 

    io_service_.post(boost::bind(&connector::call_cb, this, call_back));
}

void connector::release_connection(
    const tools::dynamic_type&          connection_type,
    void*                               connection,
    const http::response_header*        header )
{
    if ( connection_type != typeid (server::test::socket<const char*>) )
        throw std::runtime_error("test::proxy_config::release_connection: invalid type"); 

    const socket_list_t::iterator pos = std::find(
        sockets_in_use_.begin(), sockets_in_use_.end(),
        *static_cast< server::test::socket< const char* >* >( connection ) );

    assert( pos != sockets_in_use_.end() );

    if ( header )
    {
        sockets_.push_back(*pos);
    }
    else
    {
        pos->close();
    }

    sockets_in_use_.erase(pos);
}

} // namespace test
} // namespace server
