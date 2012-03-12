// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_BAYEUX_BAYEUX_H_
#define SIOUX_BAYEUX_BAYEUX_H_

#include <boost/asio/deadline_timer.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include "bayeux/response.h"
#include "pubsub/root.h"
#include "server/connection.h"
#include "json/json.h"

namespace server {
	class async_response;
	class session_generator;
}

namespace http {
	class request_header;
}

/** @namespace bayeux */
namespace bayeux
{
	/*
	 * - Session-ID generation
	 *   - IP-Addresse einbauen
	 *   - "richtigen" Zufallsgenerator wählen
	 *     - crypto- graphically random
	 *   - Session-ID wechseln, wenn Authentifizierung (warum noch mal?)
	 * - Session-ID regelmäßig wechseln
	 */

	class session;
	class configuration;

	/**
	 * @brief Class that creates responses to request targeting a bayeux server component
	 *
	 * The template is currently explicit instantiated for boost::asio::deadline_timer and server::test::timer
	 */
	template < class Timer = boost::asio::deadline_timer >
	class connector
	{
	public:
	    /**
	     * @brief this is the main interface to the bayeux component
	     *
	     * Construct a connector and response to http request that are meant to be bayeux requests
	     *
	     * @param queue a queue, that is actively used to perform IO and timeouts
	     * @param data the pubsub data root to store and retrieve data
	     * @param session_generator a crypto-graphically random generator. The access to the session generator
	     *        might by from different threads, but synchronized t one thread at a time.
	     * @param config configuration values for the bayeux protocol implementation
	     */
		connector( boost::asio::io_service& queue, pubsub::root& data, server::session_generator& session_generator,
		    const configuration& config );

		/**
		 * @brief creates a new response object for a given http request.
		 */
		template < class Connection >
		boost::shared_ptr< server::async_response > create_response(
			const boost::shared_ptr< Connection >&                    connection,
			const boost::shared_ptr< const http::request_header >&    header );

		/**
		 * @brief looks up the session with the given session_id. If no such session exists, a null pointer
		 *        is returned.
		 *
		 * If the session isn't used anymore, it has to be returned by caling idle_session().
		 */
		session* find_session( const json::string& session_id );

		/**
		 * @brief this function will create a new session with the given network_connection_name
         *
		 * If the session isn't used anymore, it has to be returned by caling idle_session().
		 *
		 * @return the returned pointer will never by null.
		 */
		session* create_session( const std::string& network_connection_name );

		/**
		 * @brief this session is currently idle
		 *
		 * This function has to be called after a session was obtained by a call to find_session() or
		 * create_session(), isn't used anymore. The session timeout starts expiring after a call to idle_session() and
		 * will stop expiring with a next call to find_session()
		 */
		void idle_session( const session* session );

        /**
         * @brief removes the session with the given id
         *
         * If no such session is given, no special action is taken.
         */
        void drop_session( const json::string& session_id );
	private:
        boost::asio::io_service&                    queue_;
		pubsub::root& 				                data_;

        boost::mutex                                mutex_;
		server::session_generator&	                session_generator_;
		boost::shared_ptr< const configuration >    current_config_;

		struct session_data
		{
		    unsigned                            use_count_;
		    bool                                remove_;
		    boost::shared_ptr< session >        session_;
		    boost::shared_ptr< Timer >          timer_;

		    session_data( const std::string& session_id, pubsub::root& data,
                const boost::shared_ptr< const configuration >& config, boost::asio::io_service& queue );
		};

		typedef std::map< std::string, session_data > session_list_t;
		session_list_t				                sessions_;

		typedef std::map< const session*, typename session_list_t::iterator > session_index_t;
		session_index_t                             index_;

		void session_timeout_reached( const session*, const boost::system::error_code& ec );

		void remove_from_sessions( typename session_list_t::iterator );
		void remove_session( const session* );
	};


	// implementation
    template < class Timer >
	template < class Connection >
	boost::shared_ptr< server::async_response > connector< Timer >::create_response(
		const boost::shared_ptr< Connection >&                    connection,
		const boost::shared_ptr< const http::request_header >&    header )
	{
		return boost::shared_ptr< server::async_response >( new response< Connection >( connection, *header, *this ) );
	}
}

#endif /* SIOUX_BAYEUX_BAYEUX_H_ */
