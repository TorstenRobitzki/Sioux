#include "asio_mocks/run.h"

namespace asio_mocks {
namespace details {

    ////////////////////////
    // class stream_decoder
    stream_decoder::stream_decoder()
        : idle_( true )
    {}

    void stream_decoder::operator()( boost::asio::const_buffer data )
    {
        for ( ; buffer_size( data ); data = feed_data( data ) )
            ;
    }

    std::vector< response_t > stream_decoder::result() const
    {
        if ( !idle_ )
            throw std::runtime_error( "incomplete http message" );

        return result_;
    }

    boost::asio::const_buffer stream_decoder::feed_data( const boost::asio::const_buffer& data )
    {
        const std::pair< bool, std::size_t > remaining = decoder_.feed_data( data );
        idle_ = remaining.first && remaining.second == 0;

        if ( remaining.first )
        {
            std::pair< boost::shared_ptr< http::response_header >, std::vector< char > > msg =
                decoder_.last_message();

            const response_t new_response = {
                msg.first,
                msg.second,
                asio_mocks::current_time()
            };

            result_.push_back( new_response );
        }

        return data + ( buffer_size( data ) - remaining.second );
    }

    void empty_call_back( const boost::system::error_code& )
    {
    }

}
}
