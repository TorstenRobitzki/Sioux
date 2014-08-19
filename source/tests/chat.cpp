#include "server/server.h"
#include "http/parser.h"
#include "file/file.h"
#include "pubsub/root.h"
#include "pubsub/pubsub.h"
#include "pubsub/node.h"
#include "pubsub/configuration.h"
#include "pubsub_http/connector.h"
#include "json_handler/response.h"
#include <iostream>

namespace
{
    typedef server::logging_server<>::connection_t connection_t;

    boost::shared_ptr< server::async_response > on_pubsub_request(
                      pubsub::http::connector<>&                        connector,
                const boost::shared_ptr< connection_t >&                connection,
                const boost::shared_ptr< const http::request_header >&  request )
    {
        const boost::shared_ptr< server::async_response > result = connector.create_response( connection, request );

        return result.get()
            ? result
            : connection->trait().error_response( connection, http::http_bad_request );
    }

    class chat_adapter : public pubsub::adapter
    {
    public:
        chat_adapter()
            : chat_channel_( pubsub::node_name().add( pubsub::key( pubsub::key_domain( "channel" ), "chat" ) ) )
            , max_size_( 20u )
            , root_( 0 )
        {
        }

        boost::shared_ptr< server::async_response > create_response(
                    const boost::shared_ptr< connection_t >&                connection,
                    const boost::shared_ptr< const http::request_header >&  request )
        {
            return boost::shared_ptr< server::async_response >( new json::response< connection_t >(
                connection, request, boost::bind( &chat_adapter::publish_message, this, _1, _2 ) ) );
        }

        void set_root( pubsub::root& root )
        {
            root_ = &root;
        }

    private:
        std::pair< json::value, http::http_error_code > publish_message( const ::http::request_header& header, const json::value& input )
        {
            const json::array messages = input.upcast< json::array >();

            if ( messages.length() != 1u )
                throw std::runtime_error( "malformed input to publish_message()");

            boost::mutex::scoped_lock lock( mutex_ );

            json::object request = messages.at( 0 ).upcast< json::object >();
            json::object decorated_entry;
            decorated_entry.add( json::string( "head" ), json::string() );
            decorated_entry.add( json::string( "text" ), request.at( "text" ) );

            chat_data_.add( decorated_entry );
            if ( chat_data_.length() > max_size_ )
                chat_data_.erase( 0, 1 );

            assert( root_ );
            root_->update_node( chat_channel_, chat_data_.copy() );

            return std::pair< json::value, http::http_error_code >( json::array(), http::http_ok );
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
            cb->initial_value( chat_data_.copy() );
        }

        boost::mutex            mutex_;
        const pubsub::node_name chat_channel_;
        const std::size_t       max_size_;
        json::array             chat_data_;
        pubsub::root*           root_;
    };

}

int main()
{
    boost::asio::io_service             queue;

    chat_adapter                        adapter;
    pubsub::root                        data( queue, adapter, pubsub::configurator().authorization_not_required() );
    adapter.set_root( data );

    pubsub::http::connector<>           pubsub_connector( queue, data );

    server::logging_server<>            server( queue, 0, std::cout );

    // routing
    server.add_action( "/pubsub", boost::bind( on_pubsub_request, boost::ref( pubsub_connector ), _1, _2 ) );
    server.add_action( "/publish", boost::bind( &chat_adapter::create_response, boost::ref( adapter ), _1, _2 ) );
    server.add_action( "/say", boost::bind( &chat_adapter::create_response, boost::ref( adapter ), _1, _2 ) );

    file::add_file_handler( server, "/jquery", boost::filesystem::canonical( __FILE__ ).parent_path() );
    file::add_file_handler( server, "/", boost::filesystem::canonical( __FILE__ ).parent_path() / "chat" );

    using namespace boost::asio::ip;
    server.add_listener( tcp::endpoint( address( address_v4::any() ), 8080u ) );
    // server.add_listener( tcp::endpoint( address( address_v6::any() ), 8080u ) );

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
