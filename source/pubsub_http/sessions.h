#ifndef SIOUX_PUBSUB_HTTP_SESSIONS_H
#define SIOUX_PUBSUB_HTTP_SESSIONS_H

#include <string>
#include <boost/thread/mutex.hpp>
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
    class sessions
    {
    public:
        explicit sessions( server::session_generator& session_generator );

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
        void idle_session( const session_impl* session );

    private:
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
        session( sessions&, const json::string& session_id, const std::string& network_connection_name );
        ~session();
        const json::string& id() const;
    private:
        // no copy, no assignment; not implemented
        session( const session& );
        session& operator=( const session& );

        sessions&           list_;
        const session_impl* impl_;
    };

}
}

#endif // include guard

