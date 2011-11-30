// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace bayeux
{
	/**
	 * @brief configuration data for a bayeux server
	 *
	 * It's not save to use one instance from multiple threads.
	 */
	class configuration
	{
	public:
		/**
		 * @brief a configuration object with all values set to defaults
		 */
		configuration();

		/**
		 * @brief returns the maximum number of subscriptions per client.
		 */
		unsigned max_subscriptions() const;

		/**
		 * @brief sets a new value for the maximum number of subscriptions per client.
		 */
		configuration& max_subscriptions( unsigned new_value );

		/**
		 * @brief maximum time, a client can be disconnected until the client will be unsubscribed and freed
		 */
		boost::posix_time::time_duration max_disconnected_time() const;

		/**
		 * @brief changes the maximum time, a client can be disconnected until the client be unsubscribed and freed
		 */
		configuration& max_disconnected_time( const boost::posix_time::time_duration& time_out );

		/**
		 * @brief maximum number of messages, that will be buffered for a client before messages will be discard.
		 *
		 * If messages have to be discard, older messages get discard first.
		 */
		unsigned max_messages_per_client() const;

		/**
		 * @brief sets a new value for the maximum messages count per client limit
		 * @post max_messages_per_client() == new_limit
		 * @post &o.max_messages_per_client( X ) == &o
		 */
		configuration& max_messages_per_client( unsigned new_limit );

		/**
		 * @brief maximum size of messages, that will be buffered for a client before messages will be discard.
		 *
		 * If the size of all stored messages exceeds this limit, messages will be dropped until the size is back to
		 * or under the limit. Earlier messages are droped first.
		 */
		std::size_t max_messages_size_per_client() const;

		/**
		 * @brief sets a new value for the maximum messages size per client limit
		 * @post max_messages_size_per_client() == new_limit
		 * @post &o.max_messages_size_per_client( X ) == &o
		 */
		configuration& max_messages_size_per_client( std::size_t new_limit );

	private:
		unsigned 							max_subscriptions_;
		boost::posix_time::time_duration	max_disconnected_time_;
		unsigned 							max_messages_per_client_;
		std::size_t							max_messages_size_per_client_;
	};

}

#endif /* CONFIGURATION_H_ */
