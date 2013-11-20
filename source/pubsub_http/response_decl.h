#ifndef SIOUX_SOURCE_PUBSUB_HTTP_RESPONSE_DECL_H
#define SIOUX_SOURCE_PUBSUB_HTTP_RESPONSE_DECL_H

#include "server/response.h"
#include "json/json.h"
#include "pubsub_http/sessions.h"
#include <boost/system/error_code.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace pubsub
{
    class root;

namespace http
{
    /**
     * @brief connection type independend part of the response implemenation
     */
    class response_base : public server::async_response
    {
    protected:
        bool check_session_or_commands_given( const json::object& message, json::string& session_id ) const;

        json::object build_response( const json::string& session_id, const json::array& response, const json::array& updates ) const;
        static bool check_node_name( const json::string cmd, const json::value& token, json::object& node_name, json::object& response );
    private:

        // async_response implementation
        virtual const char* name() const;
    };

    template < class TimeoutTimer >
    class sessions;

    /**
     * @brief class responsible to parse the input and create the protocol output
     */
    template < class Connection >
    class response : public response_base, public waiting_connection, public boost::enable_shared_from_this< response< Connection > >
    {
    public:
        response( const boost::shared_ptr< Connection >& c, sessions< typename Connection::timer_t >& s, pubsub::root& d );

        ~response();
    private:
        virtual void start();

        void body_read_handler(
            const boost::system::error_code& error,
            const char* buffer,
            std::size_t bytes_read_and_decoded );

        void protocol_body_read_handler( const json::value& );
        json::array process_commands( const json::object& message ) const;
        json::value process_command( const json::value& command ) const;

        void write_reponse( const json::object& );

        void response_written( const boost::system::error_code& ec, std::size_t size );

        void on_time_out( const boost::system::error_code& error );

        // waiting_connection implementation
        virtual void update( const json::array& response, const json::array& update );
        virtual void second_connection();

        typedef typename Connection::timer_t timer_t;
        sessions< timer_t >&                        session_list_;
        pubsub::root&                               data_;

        json::parser                                parser_;
        const boost::shared_ptr< Connection >       connection_;
        session_impl*                               session_;

        // a buffer for free texts for the http response
        std::string                                 response_buffer_;
        // a concatenated list of snippets that form the http response
        std::vector< boost::asio::const_buffer >    response_;

        timer_t                                     long_poll_timer_;
    };
}
}

#endif // include guard


