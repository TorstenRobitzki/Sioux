// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "bayeux/test_tools.h"

#include <boost/test/unit_test.hpp>

#include "bayeux/node_channel.h"
#include "http/decode_stream.h"
#include "tools/io_service.h"



json::array bayeux::test::adapter::handshakes() const
{
    boost::mutex::scoped_lock lock( mutex_ );
    return handshakes_;
}

json::array bayeux::test::adapter::publishs() const
{
    boost::mutex::scoped_lock lock( mutex_ );
    return publishs_;
}

std::pair< bool, json::string > bayeux::test::adapter::handshake( const json::value& ext, json::string& sesson_data )
{
    json::object    call;
    call.add( json::string( "ext" ), ext );
    call.add( json::string( "sesson_data" ), sesson_data );

    boost::mutex::scoped_lock lock( mutex_ );
    handshakes_.add( call );

    return std::make_pair( true, json::string() );
}

std::pair< bool, json::string > bayeux::test::adapter::publish( const json::string& channel, const json::value& data,
    const json::object& message, json::string& session_data, pubsub::root& root )
{
    json::object    call;
    call.add( json::string( "channel" ), channel );
    call.add( json::string( "data" ), data );
    call.add( json::string( "message" ), message );
    call.add( json::string( "session_data" ), session_data );

    boost::mutex::scoped_lock lock( mutex_ );
    publishs_.add( call );

    return std::make_pair( true, json::string() );
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
                    asio_mocks::current_time()
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

std::vector< bayeux::test::response_t > bayeux::test::bayeux_session( const asio_mocks::read_plan& input,
    const asio_mocks::write_plan& output, bayeux::test::context& context,
    const boost::posix_time::time_duration& timeout )
{
    stream_decoder  decoder;
    socket_t        socket( context.queue, input, output );
    socket.write_callback( boost::bind< void >( boost::ref( decoder ), _1 ) );

    typedef server::connection< bayeux::test::trait_t > connection_t;
    boost::shared_ptr< connection_t > connection( new connection_t( socket, context.trait ) );
    connection->start();

    const boost::posix_time::ptime end_of_test = asio_mocks::current_time() + timeout;

    // to wake up at the timeout time during simulation, a timer is scheduled
    asio_mocks::timer timer( context.queue );
    timer.expires_at( end_of_test );
    timer.async_wait( &empty_call_back );

    // in case that the test-setup didn't posted any handler, run() might block
    context.queue.post( boost::bind( &empty_call_back, boost::system::error_code() ) );
    do
    {
        try
        {
            tools::run( context.queue );
        }
        catch ( const std::exception& e )
        {
            std::cerr << "error running bayeux_session: " << e.what() << std::endl;
        }
        catch ( ... )
        {
            std::cerr << "error running bayeux_session" << std::endl;
        }
    }
    while ( asio_mocks::current_time() < end_of_test && asio_mocks::advance_time() != 0
        && asio_mocks::current_time() <= end_of_test );

    return decoder.result();
}

std::vector< bayeux::test::response_t > bayeux::test::bayeux_session( const asio_mocks::read_plan& input,
    bayeux::test::context& context )
{
    return bayeux_session( input, asio_mocks::write_plan(), context );
}

std::vector< bayeux::test::response_t > bayeux::test::bayeux_session( const asio_mocks::read_plan& input )
{
    bayeux::test::context   context;
    return bayeux_session( input, asio_mocks::write_plan(), context );
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

