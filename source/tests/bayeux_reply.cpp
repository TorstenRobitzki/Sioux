// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/server.h"
#include "server/test_session_generator.h"
#include "bayeux/bayeux.h"
#include "bayeux/configuration.h"
#include "bayeux/node_channel.h"
#include "bayeux/log.h"
#include "pubsub/root.h"
#include "pubsub/configuration.h"

namespace
{
    typedef server::logging_server< bayeux::stream_event_log< server::stream_event_log > > server_t;
    typedef server_t::connection_t connection_t;

    class stop_server : public std::runtime_error
    {
    public:
        stop_server() : std::runtime_error( "server was asked to stop." ) {}
    };

    boost::shared_ptr< server::async_response > on_server_stop(
                const boost::shared_ptr< connection_t >&                ,
                const boost::shared_ptr< const http::request_header >&     )
    {
        throw stop_server();
    }

    boost::shared_ptr< server::async_response > on_ping(
                const boost::shared_ptr< connection_t >&                connection,
                const boost::shared_ptr< const http::request_header >&     )
    {
        return boost::shared_ptr< server::async_response >(
                new server::error_response< connection_t >( connection, http::http_ok ) );
    }

    boost::shared_ptr< server::async_response > on_bayeux_request(
                      bayeux::connector<>&                              connector,
                const boost::shared_ptr< connection_t >&                connection,
                const boost::shared_ptr< const http::request_header >&  request )
    {
        return connector.create_response( connection, request );
    }

    class reply_adapter : public pubsub::adapter
    {
    private:
        virtual void validate_node(const pubsub::node_name&                                 node_name,
                                   const boost::shared_ptr<pubsub::validation_call_back>&   result)
        {
            result->is_valid();
        }

        virtual void authorize(const boost::shared_ptr<pubsub::subscriber>&                 client,
                               const pubsub::node_name&                                     node_name,
                               const boost::shared_ptr<pubsub::authorization_call_back>&    result)
        {
            result->is_authorized();
        }

        virtual void node_init(const pubsub::node_name&                                     node_name,
                               const boost::shared_ptr<pubsub::initialization_call_back>&   result)
        {
            result->initial_value( json::null() );
        }
    };

    struct empty {};

    class bayeux_to_pubsub_adapter : public bayeux::adapter< empty >
    {
        std::pair< bool, json::string > handshake( const json::value& ext, empty& )
        {
            return std::pair< bool, json::string >( true, json::string() );
        }

        std::pair< bool, json::string > publish( const json::string& channel, const json::value& data,
            const json::object& message, empty&, pubsub::root& root )
        {
            const pubsub::node_name node = bayeux::node_name_from_channel( channel );

            json::object reply;
            reply.add( json::string( "data" ), data );

            const json::value* const id = message.find( json::string( "id" ) );
            if ( id )
                reply.add( json::string( "id" ), *id );

            root.update_node( node, reply );
            return std::pair< bool, json::string >( true, json::string() );
        }
    };
}

int main()
{
    reply_adapter                       pubsub_adapter;
    bayeux_to_pubsub_adapter            bayeux_adapter;
    bayeux::configuration               bayeux_configuration =
        bayeux::configuration()
            .max_messages_per_client( 1000 )
            .max_messages_size_per_client( 1000 * 1000 )
            .long_polling_timeout( boost::posix_time::seconds( 60 ) );

    boost::asio::io_service             queue;
    pubsub::root                        data( queue, pubsub_adapter, pubsub::configurator().authorization_not_required() );

    server::test::session_generator     session_generator;
    bayeux::connector<>                 bayeux_connector( queue, data, session_generator, bayeux_adapter, bayeux_configuration );

    server_t server( queue, 0u, std::cout );
    using namespace boost::asio::ip;
    server.add_listener( tcp::endpoint( address( address_v4::any() ), 8080 ) );
    server.add_listener( tcp::endpoint( address( address_v6::any() ), 8080 ) );

    server.add_action( "/stop", on_server_stop );
    server.add_action( "/ping", on_ping );
    server.add_action( "/", boost::bind( on_bayeux_request, boost::ref( bayeux_connector ), _1, _2 ) );

    bool stoppping = false;
    while ( !stoppping )
    {
        try
        {
            queue.run();
        }
        catch ( const stop_server& e )
        {
            std::cout << e.what() << std::endl;
            stoppping = true;
        }
        catch ( const std::exception& e )
        {
            std::cerr << "error: " << e.what() << std::endl;
        }
        catch ( ... )
        {
            std::cerr << "unknown error." << std::endl;
        }
    }
}

