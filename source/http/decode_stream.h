// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef HTTP_DECODE_STREAM_H_
#define HTTP_DECODE_STREAM_H_

#include "http/body_decoder.h"

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

	/**
	 * @brief state full http decoder. To split a stream into separate http message headers and bodies
	 */
	template < class Message >
	class stream_decoder
	{
	public:
	    stream_decoder();

	    /**
	     * @brief feeds new data to the decoder. If a new message is completely decoded, the function
	     *        will return std::make_pair( true, undecoded ). Where undecoded is the number of bytes from
	     *        raw_data that was not passed and should be feed to the decoder with a subsequent call to feed_data().
	     *
	     * If the function returns std::make_pair( true, 0 ), than all data was consumed and a new message decoded.
	     */
	    std::pair< bool, std::size_t > feed_data( const boost::asio::const_buffer& raw_data );

	    typedef std::pair< boost::shared_ptr< Message >, std::vector< char > > message_t;

	    /**
	     * @brief returns the last, decoded http header and body
	     * @pre the last call to feed_data() must have returned true in the first member.
	     */
	    message_t last_message() const;
	private:
	    std::pair< bool, std::size_t > decode_header( const char*, std::size_t );
        std::pair< bool, std::size_t > decode_body( const char*, std::size_t );

        body_decoder decoder_;

        enum {
            idle_state,
            decoding_header,
            decoding_body
        } state_;

        message_t   current_;
	};
}

#endif /* HTTP_DECODE_STREAM_H_ */
