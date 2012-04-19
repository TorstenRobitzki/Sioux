// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "bayeux/test_tools.h"

#include <boost/test/unit_test.hpp>

#include "bayeux/node_channel.h"
#include "http/decode_stream.h"
#include "tools/io_service.h"

server::test::read bayeux::test::msg( const std::string& txt )
{
    std::string body( txt );
    std::replace( body.begin(), body.end(), '\'', '\"' );

    const std::string result =
        "POST / HTTP/1.1\r\n"
        "Host: bayeux-server.de\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: "
     + tools::as_string( body.size() )
     + "\r\n\r\n"
     +  body;

    return server::test::read( result.begin(), result.end() );
}

namespace
{
    class stream_decoder
    {
    public:
        stream_decoder()
            : idle_( true )
        {}

        void operator()( boost::asio::const_buffer data )
        {
            for ( ; buffer_size( data ); data = feed_data( data ) )
                ;
        }

        std::vector< bayeux::test::response_t > result() const
        {
            if ( !idle_ )
                throw std::runtime_error( "incomplete http message" );

            return result_;
        }
    private:
        boost::asio::const_buffer feed_data( const boost::asio::const_buffer& data )
        {
            const std::pair< bool, std::size_t > remaining = decoder_.feed_data( data );
            idle_ = remaining.first && remaining.second == 0;

            if ( remaining.first )
            {
                std::pair< boost::shared_ptr< http::response_header >, std::vector< char > > msg =
                    decoder_.last_message();

                const bayeux::test::response_t new_response = {
                    msg.first,
                    json::parse( msg.second.begin(), msg.second.end() ).upcast< json::array >(),
                    server::test::current_time()
                };

                result_.push_back( new_response );
            }

            return data + ( buffer_size( data ) - remaining.second );
        }

        http::stream_decoder< http::response_header >   decoder_;
        std::vector< bayeux::test::response_t >         result_;
        bool                                            idle_;
    };

    void empty_call_back( const boost::system::error_code& ) {}
}

std::vector< bayeux::test::response_t > bayeux::test::bayeux_session( const server::test::read_plan& input,
    const server::test::write_plan& output, bayeux::test::context& context,
    const boost::posix_time::time_duration& timeout )
{
    stream_decoder  decoder;
    socket_t        socket( context.queue, input, output );
    socket.write_callback( boost::bind< void >( boost::ref( decoder ), _1 ) );

    typedef server::connection< bayeux::test::trait_t > connection_t;
    boost::shared_ptr< connection_t > connection( new connection_t( socket, context.trait ) );
    connection->start();

    const boost::posix_time::ptime end_of_test = server::test::current_time() + timeout;

    // to wake up at the timeout time during simulation, a timer is scheduled
    server::test::timer timer( context.queue );
    timer.expires_at( end_of_test );
    timer.async_wait( &empty_call_back );

    // in case that the test-setup didn't posted any handler, run() might block
    context.queue.post( boost::bind( &empty_call_back, boost::system::error_code() ) );
    try
    {
        do
        {
            tools::run( context.queue );
        }
        while ( server::test::current_time() < end_of_test && server::test::advance_time() != 0
            && server::test::current_time() <= end_of_test );
    }
    catch ( ... )
    {
        std::cerr << "error running bayeux_session" << std::endl;
        BOOST_REQUIRE( false );
    }

    return decoder.result();
}

std::vector< bayeux::test::response_t > bayeux::test::bayeux_session( const server::test::read_plan& input,
    bayeux::test::context& context )
{
    return bayeux_session( input, server::test::write_plan(), context );
}

std::vector< bayeux::test::response_t > bayeux::test::bayeux_session( const server::test::read_plan& input )
{
    bayeux::test::context   context;
    return bayeux_session( input, server::test::write_plan(), context );
}

json::array bayeux::test::bayeux_messages( const std::vector< bayeux::test::response_t >& http_response )
{
    json::array result;

    for ( std::vector< bayeux::test::response_t >::const_iterator m = http_response.begin();
        m != http_response.end(); ++m )
    {
        result += m->second;
    }

    return result;
}

static void post_to_io_queue( boost::asio::io_service& queue, const boost::function< void() >& f )
{
    queue.post( f );
}

boost::function< void() > bayeux::test::update_node( bayeux::test::context& context, const char* channel_name,
    const json::value& data, const json::value* id )
{
    json::object message;
    message.add( json::string( "data" ), data );
    if ( id )
        message.add( json::string( "id" ), *id );

    const boost::function< void() >func = boost::bind(
            &pubsub::root::update_node,
            boost::ref( context.data ),
            bayeux::node_name_from_channel( channel_name ),
            message );

    return boost::bind( &post_to_io_queue, boost::ref( context.queue ), func );
}

