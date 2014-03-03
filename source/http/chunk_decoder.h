#ifndef SIOUX_SOURCE_HTTP_CHUNK_DECODER_H
#define SIOUX_SOURCE_HTTP_CHUNK_DECODER_H

#include "http/parser.h"
#include <stdexcept>

namespace http {

    /**
     * @brief exception, that will be thrown, if protocol violations are detected by a transfer_buffer
     */
    class chunk_decoder_parse_error : public std::runtime_error
    {
    public:
        explicit chunk_decoder_parse_error( const std::string& error_text );
    };

    /**
     * @brief class responsible to decode a chunk encoded body into a unencoded body.
     *
     * The template Parameter has to provide a function take_chunk(const char*, std::size_t) that takes decoded
     * chunks of the body and returns the number of bytes taken. The function has to take at least 1 byte per call.
     */
    template < class Sink >
    class chunk_decoder
    {
    public:
        chunk_decoder();

        /**
         * @brief feeds size bytes to the buffer. The function returns the number of bytes taken by the decoder.
         *
         * If the fed body contains a piece of body, take_chunk will called with a buffer that lies within the given
         * buffer. If take_chunk() was called, feed_chunked_buffer() will return.
         *
         * @pre chunked_done() returned false
         * @return the number of bytes takes is smaller or equal to size and will always be greater than 0.
         */
        std::size_t feed_chunked_buffer( const char* buffer, std::size_t size );

        /**
         * @brief returns true, if the whole body was feed and decoded
         */
        bool chunked_done() const;

    private:
        std::size_t                     current_chunk_;
        enum
        {
            chunk_size_start,
            chunk_size,
            chunk_extension,
            chunk_size_lf,
            chunk_data,
            chunk_data_cr,
            chunk_data_lf,
            chunk_trailer_start, // any character but \r
            chunk_trailer,
            chunk_trailer_lf,
            chunk_last_trailer_lf,
            chunk_done
        }                               chunked_state_;
    };


    // implementation
    template < class Sink >
    chunk_decoder< Sink >::chunk_decoder()
        : current_chunk_( 0 )
        , chunked_state_( chunk_size_start )
    {
    }

    template < class Sink >
    std::size_t chunk_decoder< Sink >::feed_chunked_buffer( const char* c, std::size_t org_size )
    {
        std::size_t size = org_size;
        bool delivered = false;

        for ( ; size && chunked_state_ != chunk_done && !delivered; --size, ++c )
        {
            switch ( chunked_state_ )
            {
            case chunk_size_start:
                if ( !std::isxdigit( *c ) )
                    throw chunk_decoder_parse_error( "missing chunked size" );

                current_chunk_ = http::xdigit_value( *c );
                chunked_state_ = chunk_size;
                break;
            case chunk_size:
                if ( std::isxdigit(*c) )
                {
                    if ( current_chunk_ > std::numeric_limits< std::size_t >::max() / 16 )
                        throw chunk_decoder_parse_error( "chunk size to big" );

                    current_chunk_ *= 16;
                    current_chunk_ +=  http::xdigit_value(*c);
                }
                else if ( *c == '\r' )
                {
                    chunked_state_ = chunk_size_lf;
                }
                else if ( *c == ';' )
                {
                    chunked_state_ = chunk_extension;
                }
                else
                {
                    throw chunk_decoder_parse_error( "malformed chunk-size" );
                }

                break;
            case chunk_extension:
                if ( *c == '\r' )
                    chunked_state_ = chunk_size_lf;
                break;
            case chunk_size_lf:
                {
                    if ( *c != '\n' )
                        throw chunk_decoder_parse_error( "missing linefeed in chunk size" );

                    chunked_state_ = current_chunk_ == 0 ? chunk_trailer_start : chunk_data;
                }
                break;
            case chunk_data:
                {
                    assert( current_chunk_ );
                    const size_t bite = std::min( size, current_chunk_ );

                    const size_t feed = static_cast< Sink* >( this )->take_chunk( c, bite );
                    delivered = true;

                    assert( feed <= bite );
                    assert( feed <= current_chunk_ );

                    current_chunk_ -= feed;
                    size           -= feed-1; // additional decrement in the for loop
                    c              += feed-1; // additional increment in the for loop

                    if ( current_chunk_ == 0 )
                        chunked_state_ = chunk_data_cr;
                }
                break;
            case chunk_data_cr:
                {
                    if ( *c != '\r' )
                        throw chunk_decoder_parse_error( "missing cr after chunk-data" );

                    chunked_state_ = chunk_data_lf;
                }
                break;
            case chunk_data_lf:
                {
                    if ( *c != '\n' )
                        throw chunk_decoder_parse_error( "missing lf after chunk-data" );

                    chunked_state_ = chunk_size_start;
                }
                break;
            case chunk_trailer_start:
                chunked_state_ = *c == '\r' ? chunk_last_trailer_lf : chunk_trailer;
                break;
            case chunk_trailer:
                if ( *c == '\r' )
                    chunked_state_ = chunk_trailer_lf;
                break;
            case chunk_trailer_lf:
                if ( *c != '\n' )
                    throw chunk_decoder_parse_error( "missing linefeed in trailer" );

                chunked_state_ = chunk_trailer_start;
                break;
            case chunk_last_trailer_lf:
                if ( *c != '\n' )
                    throw chunk_decoder_parse_error( "missing linefeed in trailer" );

                chunked_state_ = chunk_done;
                break;
            default:
                assert( !"invalid state" );
            }
        }

        return org_size - size;
    }

    template < class Sink >
    bool chunk_decoder< Sink >::chunked_done() const
    {
        return chunked_state_ == chunk_done;
    }


}

#endif // include guard

