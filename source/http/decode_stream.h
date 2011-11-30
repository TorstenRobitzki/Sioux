// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef HTTP_DECODE_STREAM_H_
#define HTTP_DECODE_STREAM_H_

#include <vector>
#include <utility>
#include <boost/shared_ptr.hpp>

namespace http
{
	class request_header;
	class response_header;

	/**
	 * @brief result type of decode_stream() for a request stream
	 * @relates decode_stream()
	 */
	typedef std::vector< std::pair< boost::shared_ptr< request_header >, std::vector< char > > >
		decoded_request_stream_t;

	/**
	 * @brief result type of decode_stream() for a response stream
	 * @relates decode_stream()
	 */
	typedef std::vector< std::pair< boost::shared_ptr< response_header >, std::vector< char > > >
		decoded_response_stream_t;

	/**
	 * @brief function to split http sample into headers and bodies
	 *
	 * The function splits the given character stream and splits it into
	 * a sequence of headers and bodies.
	 *
	 * If an error occures, the function will throw an exception. If not all
	 * characters could be consumed, the function will throw an exception.
	 *
	 * Message must be either http::request or http::response
	 *
	 * @relates body_decoder
	 */
	template < class Message >
	std::vector< std::pair< boost::shared_ptr< Message >, std::vector< char > > >
		decode_stream( const std::vector< char >& stream );

}

#endif /* HTTP_DECODE_STREAM_H_ */
