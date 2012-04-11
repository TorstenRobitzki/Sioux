// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/server.h"
#include "server/test_session_generator.h"
#include "bayeux/bayeux.h"
#include "bayeux/configuration.h"
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

    typedef server::basic_server< server::connection_traits<
        boost::asio::ip::tcp::socket,
        boost::asio::deadline_timer,
        response_factory,
        server::stream_event_log,
        server::stream_error_log > > server_t;

    class reply_adapter : public pubsub::adapter
    {
    private:
        virtual void validate_node(const pubsub::node_name&                                 node_name,
                                   const boost::shared_ptr<pubsub::validation_call_back>&   result)
        {
            std::cout << "node validation: " << node_name << std::endl;
            result->is_valid();
        }

        virtual void authorize(const boost::shared_ptr<pubsub::subscriber>&                 client,
                               const pubsub::node_name&                                     node_name,
                               const boost::shared_ptr<pubsub::authorization_call_back>&    result)
        {
            std::cout << "authorize requested: " << node_name << std::endl;
            result->is_authorized();
        }

        virtual void node_init(const pubsub::node_name&                                     node_name,
                               const boost::shared_ptr<pubsub::initialization_call_back>&   result)
        {
            std::cout << "node_init: " << node_name << std::endl;
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

    struct context
    {
        context( boost::asio::io_service& queue, pubsub::root& data, server::session_generator& generator )
            : connector_( queue, data, generator, bayeux::configuration() )
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
    reply_adapter                       adapter;
    pubsub::root                        data( queue, adapter, pubsub::configuration() );

    server::test::session_generator     session_generator;
    context                             parameters( queue, data, session_generator );

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

