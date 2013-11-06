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

namespace pubsub
{
namespace http
{

#if 0
    template < class Connection >
    response< Connection >::response( const boost::shared_ptr< Connection >& c, sessions< typename Connection::timer_t >& s,
        pubsub::root& d )
        : session_list_( s )
        , data_( d )
        , parser_()
        , connection_( c )
        , session_()
        , response_buffer_()
        , response_()
        , long_poll_timer_( c->socket().get_io_service() )
    {
        assert( c.get() );
    }

    template < class Connection >
    response< Connection >::~response()
    {
    }

    template < class Connection >
    void response< Connection >::start()
    {
        server::close_connection_guard< Connection > guard( *connection_, *this );

        connection_->async_read_body( boost::bind( &response::body_read_handler, this->shared_from_this(), _1, _2, _3 ) );

        guard.dismiss();
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

        if ( message.first && !message.second.empty() && check_session_or_commands_given( message.second, session_id ) )
        {
            session_.reset( new session_reference( session_list_, session_id,
                server::socket_end_point_trait< typename Connection::socket_t >::to_text( connection_->socket() ) ) );

            const json::array response = process_commands( message.second, data_, *session_ );

            long_poll_timer_.expires_from_now( long_poll_time_out );
            long_poll_timer_.async_wait( boost::bind( &response::on_time_out, this->shared_from_this(), _1 ) );

            session_->on_wake_up(
                boost::bind( &response::on_wake_up, this->shared_from_this() ),
                response,
                connection_->socket().get_io_service() );

            guard.dismiss();
        }
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

        protocol_response.to_json( response_ );
        connection_->async_write(
            response_, boost::bind( &response::response_written, this->shared_from_this(), _1, _2 ), *this );
    }

    template < class Connection >
    void response< Connection >::on_update( const json::array& response, const json::array& updates )
    {
        long_poll_timer_.cancel();

        write_reponse( build_response( session_->id(), response, updates ) );
    }

    template < class Connection >
    void response< Connection >::on_time_out( const boost::system::error_code& error )
    {
        if ( !error )
        {
            session_->wake_up();
        }
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
#endif

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
}
}
#endif // include guard

