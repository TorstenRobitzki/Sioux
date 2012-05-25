// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "http/decode_stream.h"
#include "http/request.h"
#include "http/response.h"
#include "tools/asstring.h"

#include <stdexcept>

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

        if ( stream.empty() )
            return result;

        stream_decoder< Message > decoder;

        std::pair< bool, std::size_t > state = decoder.feed_data( boost::asio::buffer( stream ) );
        while ( state.second )
        {
            if ( state.first )
                result.push_back( decoder.last_message() );

            state = decoder.feed_data( boost::asio::buffer( &stream[ 0 ] + stream.size() - state.second, state.second ) );
        }

        if ( !state.first )
            throw std::runtime_error( "unexpected stream end");

        result.push_back( decoder.last_message() );

        return result;
    }

    // explicit instantiations for both header types
	template decoded_request_stream_t decode_stream( const std::vector< char >& );
	template decoded_response_stream_t decode_stream( const std::vector< char >& );

    template < class Message >
    stream_decoder< Message >::stream_decoder()
        : state_( idle_state )
    {

    }

    template < class Message >
    std::pair< bool, std::size_t > stream_decoder< Message >::feed_data( const boost::asio::const_buffer& raw_data )
    {
        const std::size_t size = buffer_size( raw_data );
        const char* const data = boost::asio::buffer_cast< const char* >( raw_data );

        assert( size != 0 && data );

        if ( state_ == idle_state )
        {
            current_.first.reset( new Message );
            current_.second.clear();

            state_ = decoding_header;
        }

        return state_ == decoding_header
            ? decode_header( data, size)
            : decode_body( data, size );
    }

    template < class Message >
    std::pair< boost::shared_ptr< Message >, std::vector< char > > stream_decoder< Message >::last_message() const
    {
        assert( state_ == idle_state );
        return current_;
    }

    template < class Message >
    std::pair< bool, std::size_t > stream_decoder< Message >::decode_header( const char* data, std::size_t size )
    {
        Message& header = *current_.first;

        const std::pair<char*, std::size_t> write_pos = header.read_buffer();
        const std::size_t write_size = std::min( write_pos.second, size );

        std::copy( data, data + write_size, write_pos.first );
        const bool done = header.parse( write_size );

        std::size_t remaining_size = size - ( write_size - header.unparsed_buffer().second );

        if ( done )
        {
            if ( header.state() != message::ok )
                throw std::runtime_error( "error while parsing header: " + tools::as_string( header.state() ) );

            state_ = header.body_expected( http::http_post )
                ? decoding_body
                : idle_state;

            if ( state_ == decoding_body )
            {
                if ( decoder_.start( header ) != http::http_ok )
                    throw std::runtime_error( "error starting to decode message body." );

                return decode_body( data + size - remaining_size, remaining_size );
            }
        }

        return std::make_pair( done, remaining_size );
    }

    template < class Message >
    std::pair< bool, std::size_t > stream_decoder< Message >::decode_body( const char* data, std::size_t size )
    {
        if ( size == 0 )
        {
            state_ = idle_state;
            return std::make_pair( true, size );
        }

        std::vector< char >& out_buffer = current_.second;
        size -= decoder_.feed_buffer( data, size );

        for ( std::pair< std::size_t, const char* > decoded = decoder_.decode(); decoded.first;
            decoded = decoder_.decode() )
        {
            out_buffer.insert( out_buffer.end(), decoded.second, decoded.second + decoded.first );
        }

        if ( decoder_.done() )
            state_ = idle_state;

        return std::make_pair( state_ == idle_state, size );
    }

    template class stream_decoder< request_header >;
    template class stream_decoder< response_header >;

}
