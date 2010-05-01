// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_TEST_TRAIT_H
#define SIOUX_SOURCE_SERVER_TEST_TRAIT_H

#include "server/test_socket.h"

namespace server {
namespace test {

template <class Connection = server::test::socket<const char*> >
struct traits
{
	typedef Connection connection_type;

    static void create_response(const boost::shared_ptr<const server::request_header>& header)
    {
        std::cout << "Hallo, ein request..." << std::endl;
    }

    class impl;
};

} // namespace test
} // namespace server 

#endif
