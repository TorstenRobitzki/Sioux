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
	 * @brief class that keeps the pubsub::root, and the bayeux sessions together
	 *
	 * It implements a factory for responses to http requests that target a bayeux server component.
	 * @todo add support for multiple roots
	 */
	class connector
	{
	public:
		connector( boost::asio::io_service& queue, pubsub::root& data, server::session_generator& session_generator,
		    const configuration& config );

		template < class Connection >
		boost::shared_ptr< server::async_response > create_response(
			const boost::shared_ptr< Connection >&                    connection,
			const boost::shared_ptr< const http::request_header >&    header );

		/**
		 * @brief looks up the session with the given session_id. If no such session exists, a null pointer
		 *        is returned.
		 */
		boost::shared_ptr< session > find_session( const json::string& session_id );

		/**
		 * @brief this function will create a new session with the given network_connection_name
		 * @return the returned pointer will never by null.
		 */
		boost::shared_ptr< session > create_session( const std::string& network_connection_name );

		/**
		 * @brief removes the session with the given id
		 */
		void drop_session( const json::string& session_id );

		/**
		 * @brief this session is currently idle
		 *
		 * This session will be kept for at least the configured session timeout value.
		 */
		void idle_session( const boost::shared_ptr< session >& session );
	private:
		boost::asio::deadline_timer timer_;
		pubsub::root& 				data_;
		boost::mutex				generator_mutex_;
		server::session_generator&	session_generator_;

		boost::mutex				mutex_;
		boost::shared_ptr< const configuration >	current_config_;

		typedef std::map< std::string, boost::shared_ptr< session > > session_list_t;
		session_list_t				sessions_;

		void session_timeout_reached();
	};


	// implemenation
	template < class Connection >
	boost::shared_ptr< server::async_response > connector::create_response(
		const boost::shared_ptr< Connection >&                    connection,
		const boost::shared_ptr< const http::request_header >&    header )
	{
		return boost::shared_ptr< server::async_response >( new response< Connection >( connection, *header, *this ) );
	}
}

#endif /* SIOUX_BAYEUX_BAYEUX_H_ */
