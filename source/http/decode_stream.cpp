// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "http/decode_stream.h"
#include "http/request.h"
#include "http/response.h"
#include "http/body_decoder.h"

#include <stdexcept>

// delete me if you see me
#include <iostream>

namespace http
{
	namespace
	{
		template < class Message >
		std::vector< char > decode_body( const std::vector< char >& stream,
				const Message& header, std::size_t &pos )
		{
			body_decoder decoder;
			if ( decoder.start( header ) != http::http_ok )
				throw std::runtime_error( "error starting to decode message body." );

			std::vector< char > result;
			for ( ; !decoder.done(); )
			{
				if ( pos == stream.size() )
					throw std::runtime_error( "stream exhausted while decoding body.");

				pos += decoder.feed_buffer( &stream[ pos ], stream.size() - pos );

				for ( std::pair< std::size_t, const char* > part = decoder.decode();
						part.first; part = decoder.decode() )
				{
					result.insert( result.end(), part.second, part.second + part.first );
				}
			}

			return result;
		}

		template < class Message >
		void fill_header( const std::vector< char >& stream, Message& header, std::size_t& pos )
		{
	        const std::pair<char*, std::size_t> write_pos = header.read_buffer();
	        const std::size_t write_size = std::min( write_pos.second, stream.size() );

	        std::copy( stream.begin() + pos, stream.begin() + pos + write_size, write_pos.first );

			if ( !header.parse( write_size ) )
				throw std::runtime_error( "unable to parse header" );

			if ( header.state() != message::ok )
				throw std::runtime_error( "error while parsing header" );

			pos = pos + write_size - header.unparsed_buffer().second;
		}
	}

	template < class Message >
	std::vector< std::pair< boost::shared_ptr< Message >, std::vector< char > > >
		decode_stream( const std::vector< char >& stream )
	{
		std::vector< std::pair< boost::shared_ptr< Message >, std::vector< char > > > result;

		for ( std::size_t pos = 0; pos != stream.size(); )
		{
			const boost::shared_ptr< Message > new_header( new Message );
			fill_header( stream, *new_header, pos );

			const std::vector< char > new_body = new_header->body_expected( http::http_post )
				? decode_body( stream, *new_header, pos )
				: std::vector< char >();

			result.push_back( std::make_pair( new_header, new_body ) );
		}

		return result;
	}

	// explicit instantiations for both header types
	template decoded_request_stream_t decode_stream( const std::vector< char >& );
	template decoded_response_stream_t decode_stream( const std::vector< char >& );
}
