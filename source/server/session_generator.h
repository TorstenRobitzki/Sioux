// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SERVER_SESSION_GENERATOR_H_
#define SIOUX_SERVER_SESSION_GENERATOR_H_

#include <string>
#include <boost/asio/ip/tcp.hpp>

#include "tools/asstring.h"

namespace server
{
	template < class Socket >
	struct socket_end_point_trait
	{
		static std::string to_text( const Socket& s )
		{
			return tools::as_string( s.remote_endpoint() );
		}
	};

	/**
	 * @brief interface to a generator to generate session id's
	 */
	class session_generator
	{
	public:
		virtual ~session_generator() {}

		/**
		 * @brief this function is intended to generate a new session id with every call
		 * @param network_connection_name a textual representation of the client end point (ip address and port for
		 *        example). This can be used to form an own real per remote address.
		 */
		virtual std::string operator()( const char* network_connection_name ) = 0;

		/**
		 * @brief convenience overload of the function above.
		 */
		std::string operator()( const std::string& network_connection_name )
		{
			return ( *this )( network_connection_name.c_str() );
		}

		/**
		 * @brief convenience overload, to convert a Socket to a text by using socket_end_point_trait
		 */
		template < class Socket >
		std::string operator()( const Socket& s )
		{
			return (*this)( socket_end_point_trait<Socket>::to_text( s ) );
		}
	};
}

#endif /* SIOUX_SERVER_SESSION_GENERATOR_H_ */
