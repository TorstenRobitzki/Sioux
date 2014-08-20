// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/server.h"
#include "server/secure_session_generator.h"
#include "http/parser.h"
#include "file/file.h"
#include "pubsub/root.h"
#include "pubsub/configuration.h"
#include "bayeux/bayeux.h"
#include "bayeux/node_channel.h"

#include <iostream>

namespace
{
    typedef server::logging_server<>::connection_t connection_t;

    boost::shared_ptr< server::async_response > on_bayeux_request(
                      bayeux::connector<>&                              connector,
                const boost::shared_ptr< connection_t >&                connection,
                const boost::shared_ptr< const http::request_header >&  request )
    {
        return connector.create_response( connection, request );
    }

    static const pubsub::key_domain p1( "p1" );

    class chat_adapter : public bayeux::adapter< json::string >, public pubsub::adapter
    {
    public:
        chat_adapter()
            : chat_channel_( pubsub::node_name().add( pubsub::key( p1, "chat" ) ) )
            , say_channel_( pubsub::node_name().add( pubsub::key( p1, "say" ) ) )
            , max_size_( 20u )
        {
        }

    private:
        // bayeux-adapter implementation
        std::pair< bool, json::string > handshake( const json::value& ext, json::string& session )
        {
            return std::pair< bool, json::string >( true, json::string() );
        }

        bool set_name( const json::string& data, json::string& session )
        {
            const std::string nick( "/nick " );
            const std::string text = data.to_std_string();
            const std::size_t pos = text.find( nick );

            if ( pos != std::string::npos )
                session = json::string( text.substr( nick.size() ).c_str() );

            return pos != std::string::npos;
        }

        std::pair< bool, json::string > publish( const json::string& channel, const json::value& data,
            const json::object& message, json::string& session, pubsub::root& root )
        {
            const pubsub::node_name node = bayeux::node_name_from_channel( channel );

            if ( node != say_channel_ )
                return std::make_pair( false, json::string( "unexpected channel" ) );

            if ( data == json::string( "" ) || set_name( data.upcast< json::string >(), session ) )
                return std::make_pair( true, json::string() );

            boost::mutex::scoped_lock lock( mutex_ );

            json::object decorated_entry;
            decorated_entry.add( json::string( "head" ), session );
            decorated_entry.add( json::string( "text" ), data );

            chat_data_.add( decorated_entry );
            if ( chat_data_.length() > max_size_ )
                chat_data_.erase( 0, 1 );

            json::object reply;
            reply.add( json::string( "data" ), chat_data_.copy() );
            root.update_node( chat_channel_, reply );

            return std::make_pair( true, json::string() );

        }

        // pubsub-adapter implementation
        virtual void validate_node( const pubsub::node_name& node_name, const boost::shared_ptr< pubsub::validation_call_back >& cb )
        {
            if ( node_name == chat_channel_ )
                cb->is_valid();
        }

        virtual void authorize(const boost::shared_ptr< pubsub::subscriber >&,
            const pubsub::node_name& node_name, const boost::shared_ptr< pubsub::authorization_call_back >& )
        {
            assert( "authorization disabled by configuration" == 0 );
        }

        virtual void node_init(const pubsub::node_name& node_name, const boost::shared_ptr< pubsub::initialization_call_back >& cb )
        {
            cb->initial_value( chat_data_ );
        }

        boost::mutex            mutex_;
        const pubsub::node_name chat_channel_;
        const pubsub::node_name say_channel_;
        const std::size_t       max_size_;
        json::array             chat_data_;
    };

}

int main()
{
    boost::asio::io_service             queue;

    chat_adapter                        adapter;
    pubsub::root                        data( queue, adapter, pubsub::configurator().authorization_not_required() );

    server::secure_session_generator    session_generator;
    bayeux::connector<>                 bayeux( queue, data, session_generator, adapter, bayeux::configuration() );

    server::logging_server<>            server( queue, 0, std::cout );

    server.add_action( "/bayeux", boost::bind( on_bayeux_request, boost::ref( bayeux ), _1, _2 ) );
    file::add_file_handler( server, "/jquery", boost::filesystem::canonical( __FILE__ ).parent_path() );
    file::add_file_handler( server, "/", boost::filesystem::canonical( __FILE__ ).parent_path() / "bayeux_chat" );

    using namespace boost::asio::ip;
	// JRL -- comment out for boost 1_54 support otherwise an exception is thrown "bind: Address already in use"
    // server.add_listener( tcp::endpoint( address( address_v4::any() ), 8080u ) );
    server.add_listener( tcp::endpoint( address( address_v6::any() ), 8080u ) );

    std::cout << "browse for \"http://localhost:8080/\"" << std::endl;

    for ( ;; )
    {
        try
        {
            queue.run();
        }
        catch ( const std::exception& e )
        {
            std::cerr << "error: " << e.what() << std::endl;
        }
    }
}
