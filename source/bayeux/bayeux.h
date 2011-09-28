// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_BAYEUX_BAYEUX_H_
#define SIOUX_BAYEUX_BAYEUX_H_

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/shared_ptr.hpp>

#include "pubsub/root.h"

namespace server {
	class async_response;
}

namespace http {
	class request_header;
}

namespace pubsub {
	class root;
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

	/**
	 * @brief configuration data for a bayeux server
	 */
	class configuration
	{
	public:
		/**
		 * @brief returns the maximum number of subscriptions per client.
		 */
		unsigned max_subscriptions() const;

		/**
		 * @brief sets a new value for the maximum number of subscriptions per client.
		 */
		void max_subscriptions( unsigned new_value ) const;

		/**
		 * @brief maximum time, a client can be disconnected until the client will be unsubscribed and freed
		 */
		boost::posix_time::time_duration max_disconnected_time() const;
	private:
		unsigned 							max_subscriptions_;
		boost::posix_time::time_duration	max_disconnected_time_;
	};

	class connector
	{
	public:
		explicit connector( const configuration& config );

		template < class Connection >
		boost::shared_ptr< server::async_response > create_response(
			const boost::shared_ptr< Connection >&                    connection,
			const boost::shared_ptr< const http::request_header >&    header );
	private:
		pubsub::root& data_;
	};
}

#endif /* SIOUX_BAYEUX_BAYEUX_H_ */
