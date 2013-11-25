#ifndef SIOUX_SOURCE_PUBSUB_HTTP_RESPONSE_H
#define SIOUX_SOURCE_PUBSUB_HTTP_RESPONSE_H

#include "pubsub_http/response_decl.h"
#include "pubsub_http/sessions.h"
#include "json/json.h"
#include "http/server_header.h"
#include "tools/asstring.h"
#include "server/session_generator.h"
#include <vector>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/tuple/tuple.hpp>

namespace pubsub
{
namespace http
{
    namespace internal {

        extern const json::string id_token;
        extern const json::string cmd_token;
        extern const json::string response_token;
        extern const json::string error_token;
        extern const json::string key_token;
        extern const json::string update_token;
        extern const json::string from_token;
        extern const json::string data_token;
        extern const json::string version_token;

        extern const json::string subscribe_token;
        extern const json::string unsubscribe_token;

    }

    template < class Connection >
    response< Connection >::response( const boost::shared_ptr< Connection >& c, sessions< typename Connection::timer_t >& s,
        pubsub::root& d )
        : session_list_( s )
        , data_( d )
        , parser_()
        , connection_( c )
        , session_( 0 )
        , response_buffer_()
        , response_()
        , long_poll_timer_( c->socket().get_io_service() )
    {
        assert( c.get() );
    }

    template < class Connection >
    response< Connection >::~response()
    {
        if ( session_ )
            session_list_.idle_session( session_ );
    }

    template < class Connection >
    void response< Connection >::start()
    {
        server::close_connection_guard< Connection > guard( *connection_, *this );

        connection_->async_read_body( boost::bind( &response::body_read_handler, this->shared_from_this(), _1, _2, _3 ) );

        guard.dismiss();
    }

    template < class Connection >
    void response< Connection >::implement_hurry()
    {
        if ( session_ && session_list_.wake_up( session_, this->shared_from_this() ) )
            update( json::array(), json::array() );
    }

    template < class Connection >
    void response< Connection >::body_read_handler(
        const boost::system::error_code& error,
        const char* buffer,
        std::size_t bytes_read_and_decoded )
    {
        server::close_connection_guard< Connection > guard( *connection_, *this );

        if ( !error )
        {
            if ( bytes_read_and_decoded == 0 )
            {
                parser_.flush();
                const json::value protocol = parser_.result();
                guard.dismiss();

                protocol_body_read_handler( protocol );
            }
            else
            {
                parser_.parse( buffer, buffer + bytes_read_and_decoded );

                guard.dismiss();
            }
        }
    }

    template < class Connection >
    void response< Connection >::protocol_body_read_handler( const json::value& p )
    {
        static const boost::posix_time::time_duration long_poll_time_out( boost::posix_time::seconds( 20u ) );

        server::report_error_guard< Connection > guard( *connection_, *this, ::http::http_bad_request );

        const std::pair< bool, json::object > message = p.try_cast< json::object >();
        json::string  session_id;

        if ( message.first && check_session_or_commands_given( message.second, session_id ) )
        {
            bool new_session = false;

            boost::tie( session_, new_session ) = session_list_.find_or_create_session( session_id,
                server::socket_end_point_trait< typename Connection::socket_t >::to_text( connection_->socket() ) );

            const json::array response = process_commands( message.second );

            json::array stored_updates, stored_response;

            if ( new_session || !response.empty() || session_list_.pending_updates( session_, stored_updates, stored_response ) )
            {
                write_reponse( build_response( id( session_ ), response + stored_response, stored_updates ) );
            }
            else
            {
                long_poll_timer_.expires_from_now( long_poll_time_out );
                long_poll_timer_.async_wait( boost::bind( &response::on_time_out, this->shared_from_this(), _1 ) );

                session_list_.wait_for_session_event( session_, this->shared_from_this() );
            }

            guard.dismiss();
        }
    }

    template < class Connection >
    json::value response< Connection >::process_command( const json::value& command ) const
    {
        json::object cmd = command.upcast< json::object >();
        json::object response;
        json::object node;

        const json::value* const subscribe = cmd.find( internal::subscribe_token );
        if ( subscribe )
        {
            if ( !check_node_name( internal::subscribe_token, *subscribe, node, response ) )
                return response;

            session_list_.subscribe( session_, node_name( node ) );
            return json::null();
        }

        const json::value* const unsubscribe = cmd.find( internal::unsubscribe_token );
        if ( unsubscribe )
        {
            if ( !check_node_name( internal::unsubscribe_token, *unsubscribe, node, response ) )
                return response;

            if ( session_list_.unsubscribe( session_, node_name( node ) ) != 1u )
            {
                static const json::string unsubscribe_error_message( "not subscribed" );
                response.add( internal::unsubscribe_token, node );
                response.add( internal::error_token, unsubscribe_error_message );
                return response;
            }

            return json::null();
        }

        return json::null();
    }

    template < class Connection >
    json::array response< Connection >::process_commands( const json::object& message ) const
    {
        assert( session_ );
        const json::value* const cmd_field = message.find( internal::cmd_token );

        json::array result;

        if ( !cmd_field )
            return result;

        const json::array commands = cmd_field->upcast< json::array >();

        for ( std::size_t cmd_idx = 0, num_cmds = commands.length(); cmd_idx != num_cmds; ++cmd_idx )
        {
            const json::value response = process_command( commands.at( cmd_idx ) );

            if ( response != json::null() )
                result.add( response );
        }

        return result;
    }

    template < class Connection >
    void response< Connection >::write_reponse( const json::object& protocol_response )
    {
        static const char response_header[] =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            SIOUX_SERVER_HEADER
            "Content-Length: ";

        response_buffer_ = tools::as_string( protocol_response.size() ) + "\r\n\r\n";

        response_.push_back( boost::asio::buffer( response_header, sizeof response_header -1 ) );
        response_.push_back( boost::asio::buffer( response_buffer_ ) );

        // keep a copy of the protocol_response, as response_ contains just pointers into the json_response_
        json_response_ = protocol_response;
        json_response_.to_json( response_ );
        connection_->async_write(
            response_, boost::bind( &response::response_written, this->shared_from_this(), _1, _2 ), *this );
    }

    template < class Connection >
    void response< Connection >::on_time_out( const boost::system::error_code& error )
    {
        if ( !error && session_list_.wake_up( session_, this->shared_from_this() ) )
        {
            write_reponse( build_response( id( session_ ), json::array(), json::array() ) );
        }
    }

    template < class Connection >
    void response< Connection >::update( const json::array& response, const json::array& updates )
    {
        long_poll_timer_.cancel();

        write_reponse( build_response( id( session_ ), response, updates ) );
    }

    template < class Connection >
    void response< Connection >::second_connection()
    {
        update( json::array(), json::array() );
    }

    template < class Connection >
    void response< Connection >::response_written( const boost::system::error_code& ec, std::size_t size )
    {
        if ( ec )
        {
            connection_->response_not_possible( *this );
        }
        else
        {
            connection_->response_completed( *this );
        }
    }
}
}
#endif // include guard

