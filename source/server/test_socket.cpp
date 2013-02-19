// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/test_socket.h"

namespace server
{
namespace test
{

	boost::asio::ip::tcp::endpoint socket_base::remote_endpoint() const
	{
		return boost::asio::ip::tcp::endpoint( boost::asio::ip::address_v4::from_string( "192.168.210.1" ), 9999 );
	}

    boost::asio::ip::tcp::endpoint socket_base::remote_endpoint( boost::system::error_code & ec ) const
    {
        return remote_endpoint();
    }


} // namespace test
} // namespace server
