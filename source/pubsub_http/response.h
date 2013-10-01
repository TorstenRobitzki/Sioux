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

    template < class Connection >
    response< Connection >::response( const boost::shared_ptr< Connection >& c, sessions& s )
        : session_list_( s )
        , parser_()
        , connection_( c )
        , response_buffer_()
        , response_()
    {
        assert( c.get() );
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
        server::report_error_guard< Connection > guard( *connection_, *this, ::http::http_bad_request );

        const std::pair< bool, json::object > message = p.try_cast< json::object >();
        json::string  session_id;

        if ( message.first && !message.second.empty() && check_session_or_commands_given( message.second, session_id ) )
        {
            const session current_session( session_list_, session_id,
                server::socket_end_point_trait< typename Connection::socket_t >::to_text( connection_->socket() ) );

            const json::array response = process_commands( message.second );

            write_reponse( build_response( current_session.id(), response ) );

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

    namespace internal {

        extern const json::string id_token;
        extern const json::string cmd_token;
        extern const json::string response_token;
        extern const json::string error_token;

        extern const json::string subscribe_token;
        extern const json::string unsubscribe_token;

    }
}
}

#endif // include guard

