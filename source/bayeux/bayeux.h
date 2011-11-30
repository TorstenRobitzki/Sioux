// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_BAYEUX_BAYEUX_H_
#define SIOUX_BAYEUX_BAYEUX_H_

#include <boost/shared_ptr.hpp>
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
		connector( pubsub::root& data, server::session_generator& session_generator, const configuration& config );

		template < class Connection >
		boost::shared_ptr< server::async_response > create_response(
			const boost::shared_ptr< Connection >&                    connection,
			const boost::shared_ptr< const http::request_header >&    header );


		boost::shared_ptr< session > find_session( const json::string& session_id );

		/**
		 * @brief this function will create a new session with the given network_connection_name
		 * @return the returned pointer will never by null.
		 */
		boost::shared_ptr< session > create_session( const std::string& network_connection_name );

		/**
		 * @brief removes the session with the given id
		 *
		 * If now such id is known,
		 */
		void drop_session( const std::string& sesion_id );
	private:
		pubsub::root& 				data_;
		boost::mutex				generator_mutex_;
		server::session_generator&	session_generator_;

		boost::mutex				mutex_;
		boost::shared_ptr< const configuration >	current_config_;

		typedef std::map< std::string, boost::shared_ptr< session > > session_list_t;
		session_list_t				sessions_;
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
