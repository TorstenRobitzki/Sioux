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
    class response_factory
    {
    public:
        template < class Context >
        explicit response_factory( const Context& connector )
          : connector_( connector.connector_ )
        {
        }

        template < class Connection >
        boost::shared_ptr< server::async_response > create_response(
            const boost::shared_ptr< Connection >&                    connection,
            const boost::shared_ptr< const http::request_header >&    header )
        {
            if ( header->state() == http::message::ok )
            {
                return connector_.create_response( connection, header );
            }

            return boost::shared_ptr< server::async_response >(
                    new server::error_response< Connection >( connection, http::http_bad_request ) );
        }

        template < class Connection >
        boost::shared_ptr< server::async_response > error_response(
            const boost::shared_ptr< Connection >&  con,
            http::http_error_code                   ec )
        {
            return boost::shared_ptr< server::async_response >( new ::server::error_response< Connection >( con, ec ) );
        }
    private:
        bayeux::connector<>& connector_;
    };

    typedef server::basic_server<
        server::connection_traits<
            boost::asio::ip::tcp::socket,
            boost::asio::deadline_timer,
            response_factory,
            bayeux::stream_event_log< server::null_event_logger >,
            server::stream_error_log > > server_t;

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

        virtual void invalid_node_subscription(const pubsub::node_name&                     node,
                                               const boost::shared_ptr<pubsub::subscriber>& )
        {
            std::cout << "!!!!invalid_node_subscription: " << node << std::endl;
        }

        virtual void unauthorized_subscription(const pubsub::node_name& node,
                                               const boost::shared_ptr<pubsub::subscriber>&)
        {
            std::cout << "!!!!unauthorized_subscription: " << node << std::endl;
        }


        virtual void initialization_failed(const pubsub::node_name& node)
        {
            std::cout << "!!!!initialization_failed: " << node << std::endl;
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

    struct context
    {
        context( boost::asio::io_service& queue, pubsub::root& data, server::session_generator& generator,
            bayeux_to_pubsub_adapter& adapter )
            : connector_( queue, data, generator, adapter,
                bayeux::configuration()
                    .max_messages_per_client( 1000 )
                    .max_messages_size_per_client( 1000 * 1000 )
                    .long_polling_timeout( boost::posix_time::seconds( 60 ) ) )
        {
        }

        std::ostream& logstream() const
        {
            return std::cout;
        }

        mutable bayeux::connector<> connector_;
    };

}

int main()
{
    boost::asio::io_service             queue;
    reply_adapter                       pubsub_adapter;
    pubsub::root                        data( queue, pubsub_adapter, pubsub::configurator().authorization_not_required() );

    server::test::session_generator     session_generator;
    bayeux_to_pubsub_adapter            bayeux_adapter;
    context                             parameters( queue, data, session_generator, bayeux_adapter );

    server_t server( queue, 0u, parameters );
    using namespace boost::asio::ip;
    server.add_listener( tcp::endpoint( address( address_v4::any() ), 8080 ) );
    server.add_listener( tcp::endpoint( address( address_v6::any() ), 8080 ) );

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
        catch ( ... )
        {
            std::cerr << "unknown error." << std::endl;
        }
    }
}

