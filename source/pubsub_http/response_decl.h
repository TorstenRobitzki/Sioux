#ifndef SIOUX_SOURCE_PUBSUB_HTTP_RESPONSE_DECL_H
#define SIOUX_SOURCE_PUBSUB_HTTP_RESPONSE_DECL_H

#include "server/response.h"
#include "json/json.h"
#include <boost/system/error_code.hpp>

namespace pubsub
{
namespace http
{
    /**
     * @brief connection type independend part of the response implemenation
     */
    class response_base : public server::async_response
    {
    protected:
        bool check_session_or_commands_given( const json::object& message, json::string& session_id ) const;
        json::array process_commands( const json::object& message ) const;
        json::object build_response( const json::string& session_id, const json::array& response ) const;

    private:
        json::value process_command( const json::value& command ) const;
        virtual const char* name() const;
    };

    template < class TimeoutTimer >
    class sessions;

    /**
     * @brief class responsible to parse the input and create the protocol output
     */
    template < class Connection >
    class response : public response_base, public boost::enable_shared_from_this< response< Connection > >
    {
    public:
        response( const boost::shared_ptr< Connection >& c, sessions< typename Connection::timer_t >& s );
    private:

        virtual void start();

        void body_read_handler(
            const boost::system::error_code& error,
            const char* buffer,
            std::size_t bytes_read_and_decoded );

        void protocol_body_read_handler( const json::value& );

        void write_reponse( const json::object& );

        void response_written( const boost::system::error_code& ec, std::size_t size );

        sessions< typename Connection::timer_t >&   session_list_;

        json::parser                                parser_;
        const boost::shared_ptr< Connection >       connection_;

        // a buffer for free texts for the http response
        std::string                                 response_buffer_;
        // a concatenated list of snippets that form the http response
        std::vector< boost::asio::const_buffer >    response_;
    };
}
}

#endif // include guard


