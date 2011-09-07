// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_IP_PROXY_H
#define SIOUX_SOURCE_SERVER_IP_PROXY_H

#include "server/proxy.h"
#include "server/proxy_connector.h"
#include "server/proxy_response.h"

namespace server {

    template <class Socket>
    class ip_proxy
    {
    public:
        ip_proxy(
            boost::asio::io_service&                               queue, 
            const boost::shared_ptr<const proxy_configuration>&    config, 
            const boost::asio::ip::tcp::endpoint&                  ep);

        template <class Connection>
        boost::shared_ptr<async_response> create_response(
            const boost::shared_ptr<Connection>&                    connection,
            const boost::shared_ptr<const http::request_header>&    header);

    private:
        boost::shared_ptr<ip_proxy_connector<Socket> >  connector_;
        boost::asio::io_service&                        queue_;
        boost::shared_ptr<const proxy_configuration>    config_;
    };

    //////////////////////
    // implementation
    template <class Socket>
    ip_proxy<Socket>::ip_proxy(
        boost::asio::io_service&                               queue, 
        const boost::shared_ptr<const proxy_configuration>&    config, 
        const boost::asio::ip::tcp::endpoint&                  ep)
        : connector_(new ip_proxy_connector<Socket>(queue, config, ep))
        , queue_(queue)
        , config_(config)
    {
    }

    template <class Socket>
    template <class Connection>
    boost::shared_ptr<async_response> ip_proxy<Socket>::create_response(
        const boost::shared_ptr<Connection>&                    connection,
        const boost::shared_ptr<const http::request_header>&    header)
    {
        boost::shared_ptr<async_response> result(
            new proxy_response<Connection>(connection, header, *connector_, queue_, config_));

        return result;
    }

} // namespace server

#endif // include guard

