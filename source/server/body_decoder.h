// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SERVER_BODY_DECODER_H_
#define SERVER_BODY_DECODER_H_

#include "http/message.h"
#include "http/http.h"
#include "http/parser.h"

namespace server
{
	/**
	 * @brief decodes a message body
	 *
	 * @todo add chunked bodies
	 * @todo add compression
	 */
	class body_decoder
	{
	public:
        /**
         * @brief initialized this buffer for decoding a new message body
         * @return http::http_ok, if the decoder can decode the given message
         */
        template < class Base >
        http::http_error_code start( const http::message_base< Base >& request );

        /**
         * @brief feeds a new part of the body to the decoder.
         *
         * It's assumed, that the passed buffer stays valid until the whole
         * buffer is consumed by call to decode(). The function returns the
         * number of bytes taken from the input stream.
         */
        std::size_t feed_buffer( const char* buffer, std::size_t size );

        /**
         * @brief returns true, if the whole body was feed() and decode()'d
         */
        bool done() const;

        /**
         * @brief returns a part of the decodes body. If the first member of
         * the returned pair is 0, the part, that was feed to the decode by feed_buffer()
         * is decoded and more data must be feeded to the decoder.s
         */
        std::pair< std::size_t, const char* > decode();

	private:
        void start_content_length_encoded( std::size_t size );

        std::size_t	total_size_;
        std::size_t	current_size_;
        const char*	current_;
	};

	///////////////////
	// implementation
    template < class Base >
    http::http_error_code body_decoder::start( const http::message_base< Base >& request )
    {
    	const http::header* length_header = request.find_header( "Content-Length" );
    	if ( !length_header )
    		return http::http_length_required;

    	std::size_t length = 0;
    	if ( !http::parse_number( length_header->value().begin(), length_header->value().end(), length ) )
    		return http::http_bad_request;

    	start_content_length_encoded( length );

    	return http::http_ok;
    }
}

#endif /* SERVER_BODY_DECODER_H_ */
