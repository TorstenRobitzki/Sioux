// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/connection.h"
#include "server/traits.h"
#include "server/server.h"
#include <iostream>
#include <exception>
#include <boost/asio.hpp>

int main()
{
    using namespace boost::asio::ip;

    std::cout << "this is Sioux 0.1" << std::endl;

    try
    {
        boost::asio::io_service     queue;
        server::logging_server<>    sioux(queue, 0, std::clog);

        sioux.add_listener(tcp::endpoint(address(address_v4::any()), 80));
        sioux.add_listener(tcp::endpoint(address(address_v6::any()), 80));

        tcp::resolver          resolver(queue);
        tcp::resolver::query   query("robitzki.de", "http");

        for ( tcp::resolver_iterator  begin = resolver.resolve(query), end; begin != end; ++begin )
            sioux.add_proxy("/", *begin, server::proxy_configuration());

        queue.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "error: " << e.what() << std::endl;
    }
}