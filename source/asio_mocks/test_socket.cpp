#include "asio_mocks/test_socket.h"

namespace asio_mocks
{

	boost::asio::ip::tcp::endpoint socket_base::remote_endpoint() const
	{
		return boost::asio::ip::tcp::endpoint( boost::asio::ip::address_v4::from_string( "192.168.210.1" ), 9999 );
	}

    boost::asio::ip::tcp::endpoint socket_base::remote_endpoint( boost::system::error_code & ec ) const
    {
        return remote_endpoint();
    }


} // namespace asio_mocks
