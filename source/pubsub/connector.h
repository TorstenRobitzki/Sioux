// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_PUBSUB_CONNECTOR_H_
#define SIOUX_PUBSUB_CONNECTOR_H_

#include <boost/asio/io_service.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/shared_ptr.hpp>

namespace http {
	class request_header;
}

namespace server {
	class async_response;
}

namespace pubsub {

	class root;

	/*!
	 * @brief empty configuration class for configurations to come
	 */
	class connector_configuration
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

	/*!
	 * @protocol
	 *
	 * Add subscription
	 * { "subscribe" : <node> [, "version" : <version>] [, "SIOUXID" : <session id>] }
	 *
	 * Remove subscription
	 * { "unsubscribe" : <node>, "SIOUXID" : <session id> }
	 *
	 * Change data
	 * { "change" : <single-node>, "data" : <data> [, "SIOUXID" : <session id>]  }
	 *
	 * Error-Response
	 * "error" : { "code" : 42, "text" : "you are not allowed to subscribe to : xxx"
	 *
	 * Data-Response
	 * "data" : [ { "update" : <delta-encoded-json>, "version" : <version> }, { "data" : <json>, "version" : <version> } ]
	 *
	 * Idle-Response
	 * "connection" : "idle"
	 *
	 * Common-Response
	 * "SIOUXID" : <session id>
	 *
	 * <single-node> = { "foo" : 4, "bar" : false }
	 * <node> = <single-node> | [{ "foo" : 4, "bar" : false }, { "other": "blabla" }]
	 * <version> = 42 | [42 , 12]
	 * <session id> = 128bit base64 encoded session id
	 */

	/*!
	 * @brief factory to create responses to requests to a pubsub node
	 *
	 * - generates Session IDs
	 */
	class connector
	{
	public:
		connector( root& r, const connector_configuration& config );

		template < class Connection >
		boost::shared_ptr< server::async_response > create_response(
			const boost::shared_ptr< Connection >&                    connection,
			const boost::shared_ptr< const http::request_header >&    header );

	private:
		const connector_configuration	config_;
		root&							root_;
	};

	template < class Connection >
	class idle_response : public server::async_response
	{
	public:
	private:
        virtual void implement_hurry();
        virtual void start() = 0;
	};

	////////////////////////////////////
    // implementation
}

#endif /* SIOUX_PUBSUB_CONNECTOR_H_ */
