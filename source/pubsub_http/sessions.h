#ifndef SIOUX_PUBSUB_HTTP_SESSIONS_H
#define SIOUX_PUBSUB_HTTP_SESSIONS_H

#include <string>
#include <boost/thread/mutex.hpp>
#include <boost/function.hpp>
#include <boost/asio/io_service.hpp>
#include "json/json.h"

namespace server {
    class session_generator;
}

namespace pubsub {
namespace http {


    class session;
    class session_impl;

    /**
     * @brief class responsible to keep a list of active sessions
     */
    template < class TimeoutTimer >
    class sessions
    {
    public:
        sessions( server::session_generator& session_generator, boost::asio::io_service& queue );

        ~sessions();

        /**
         * @brief looks up the session with the given session_id. If no such session exists, a new session will be
         *        generated, using the given network_connection_name.
         *
         * If the returned session isn't used anymore, it has to be returned by caling idle_session().
         *
         * @return a pointer to a session, that will nevery be null.
         */
        session_impl* find_or_create_session( const json::string& session_id, const std::string& network_connection_name );

        /**
         * @brief this session is currently idle
         *
         * This function has to be called after a session was obtained by a call to find_or_create_session(), isn't
         * used anymore. The session timeout starts expiring after a call to idle_session() and
         * will stop expiring with a next call to find_or_create_session().
         */
        void idle_session( session_impl* session );

        /**
         * @brief prepares shut down by timing out all existing sessions and by timing out all new connections.
         */
        void shut_down();

    private:
        void setup_timeout( session_impl* session );
        void cancel_timer( session_impl* session );

        /**
         * @brief the session was not used for a longer period and will be deleted
         */
        void timeout_session( const json::string& session_id, const boost::system::error_code& error );

        boost::asio::io_service&    queue_;

        typedef std::map< json::string, session_impl* > session_list_t;

        boost::mutex                mutex_;
        session_list_t              sessions_;
        server::session_generator&  session_generator_;
    };

    /**
     * @brief interface to response object
     */
    class session
    {
    public:
        template < class TimeoutTimer >
        session( sessions< TimeoutTimer >&, const json::string& session_id, const std::string& network_connection_name );
        ~session();
        const json::string& id() const;
    private:
        // no copy, no assignment; not implemented
        session( const session& );
        session& operator=( const session& );

        session_impl*               impl_;
        boost::function< void() >   release_;
    };

}
}

#endif // include guard

