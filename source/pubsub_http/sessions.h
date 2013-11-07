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
    class root;
    class node_name;

namespace http {


    /**
     * @brief interface that expands the pubsub::subscriber interface to allow the comunication, of a second listener
     */
    class waiting_connection
    {
    public:
        virtual void update( const json::array& response, const json::array& update ) = 0;

        /**
         * @brief when this function is called, there was a second connection wainting for events from this session.
         */
        virtual void second_connection() = 0;
    };

    /**
     * @brief opaque type to reference to a session
     */
    class session_impl;

    /**
     * @brief class responsible to keep a list of active sessions
     */
    template < class TimeoutTimer >
    class sessions
    {
    public:
        sessions( server::session_generator& session_generator, boost::asio::io_service& queue, pubsub::root& root );

        ~sessions();

        /**
         * @brief looks up the session_impl with the given session_id. If no such session exists, a new session will be
         *        generated, using the given network_connection_name.
         *
         * If the returned session_impl isn't used anymore, it has to be returned by caling idle_session().
         *
         * @return a pointer to a session, that will nevery be null. If the returned bool is true, the session was created.
         */
        std::pair< session_impl*, bool > find_or_create_session( const json::string& session_id, const std::string& network_connection_name );

        /**
         * @brief this session_reference is currently idle
         *
         * This function has to be called after a session was obtained by a call to find_or_create_session(), isn't
         * used anymore.
         */
        void idle_session( session_impl* session );

        /**
         * @brief keeps the connection assoziated with the session_impl, until an event occures, or wake_up() gets called.
         *
         * Once wait_for_session_event() was called, the passed connection will be informed about updates.
         *
         * @pred session must have been aquired by a call to find_or_create_session() and idle_session() wasn't called jet.
         */
        void wait_for_session_event( session_impl* session, const boost::shared_ptr< waiting_connection >& connection );

        /**
         * @brief removes an assoziation with a connection from the given session.
         *
         * If the function returns true, the given connection was removed from the session. If the function returns
         * false, the connection was already removed.
         *
         * If the function returned false, update() or second_connection() will be called in the near future, or was
         * already called.
         */
        bool wake_up( session_impl* session, const boost::shared_ptr< waiting_connection >& connection );

        /**
         * @brief subscribes the given session to the given node
         */
        void subscribe( session_impl* session, const node_name& node_name );

        /**
         * @brief unsubscribes the given session from the given node
         *
         * The function returns true, if the client was subscribed to the node and false if the client was not
         * subscribed. In both cases, the client is now unsubscribed from the given node.
         */
        bool unsubscribe( session_impl* session, const node_name& node_name );

        /**
         * @brief prepares shut down by timing out all existing sessions and by timing out all new connections.
         */
        void shut_down();

    private:
        void setup_timeout( session_impl& session );
        void cancel_timer( session_impl& session );

        /**
         * @brief the session_reference was not used for a longer period and will be deleted
         */
        void timeout_session( const json::string& session_id, const boost::system::error_code& error );

        boost::asio::io_service&    queue_;
        pubsub::root&               root_;

        typedef std::map< json::string, boost::shared_ptr< session_impl > > session_list_t;

        boost::mutex                mutex_;
        session_list_t              sessions_;
        server::session_generator&  session_generator_;
    };

    /**
     * @brief returns the id to a session
     * @relates session_impl
     */
    json::string id( session_impl* );

}
}

#endif // include guard

