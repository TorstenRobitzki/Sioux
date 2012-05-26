// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/server.h"
#include "file/file.h"
#include <iostream>

int main()
{
    boost::asio::io_service queue;
    server::logging_server<> server( queue, 0, std::cout );

    file::add_file_handler( server, "/", boost::filesystem::canonical( __FILE__ ).parent_path() );

    using namespace boost::asio::ip;
    server.add_listener( tcp::endpoint( address( address_v4::any() ), 8080u ) );
    server.add_listener( tcp::endpoint( address( address_v6::any() ), 8080u ) );

    std::cout << "browse for \"http://127.0.0.1:8080/\"" << std::endl;

    for ( ;; )
        queue.run();
}
