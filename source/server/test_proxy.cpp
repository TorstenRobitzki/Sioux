// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/test_proxy.h"
#include <typeinfo>

namespace server
{
namespace test {

//////////////////////
// class proxy_config 
proxy_config::proxy_config(const std::string& simulate_response)
 : simulate_response_(simulate_response.begin(), simulate_response.end())
 , socket_(&simulate_response_[0], &simulate_response_[0] + simulate_response_.size())
 , socket_in_use_(false)
{
}

proxy_config::proxy_config(const socket<const char*>& socket)
 : simulate_response_()
 , socket_(socket)
 , socket_in_use_(false)
{
}

proxy_config::~proxy_config()
{
    if ( socket_in_use_ )
        throw std::runtime_error("server::test::proxy_config: socket still in use!");
}

std::string proxy_config::received() const
{
    return socket_.output();
}

bool proxy_config::process()
{
    if ( call_back_.get() )
    {
        call_back_->connection_received(&socket_, make_error_code(boost::system::errc::success));
        call_back_.release();

        return true;
    }

    return false;
}

void proxy_config::async_get_proxy_connection(
    const tools::dynamic_type&          connection_type,
    const std::string&                  /* orgin */,
    std::auto_ptr<connect_callback>&    call_back)
{
    if ( connection_type != typeid (server::test::socket<const char*>) )
        throw std::runtime_error("test::proxy_config::async_get_proxy_connection: invalid type"); 

    call_back_ = call_back;
}

void proxy_config::release_connection(
    const tools::dynamic_type&          connection_type,
    void*                               connection)
{
    if ( connection_type != typeid (server::test::socket<const char*>) )
        throw std::runtime_error("test::proxy_config::release_connection: invalid type"); 

    assert(connection == &socket_);
}

} // namespace test
} // namespace server
