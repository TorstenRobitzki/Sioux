// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_PUBSUB_STRESS_SUBSCRIBER_H
#define SIOUX_PUBSUB_STRESS_SUBSCRIBER_H

#include "pubsub/pubsub.h"
#include "pubsub/root.h"
#include "pubsub/configuration.h"

#include <boost/enable_shared_from_this.hpp>

namespace pubsub
{
	class root;

	namespace test
	{
		/*
		 * actions to be performed
		 * - subscribe to a valid node
		 * - unsubscribe from a subscribed node
		 * - unsubscribe from a not subscribed node
		 * - subscribe to an invalid node
		 * - subscribe to an authorized node
		 * - subscribe to a node that the user is not authorized
		 * - change authorization configuration
		 */
		class stress_subscriber : public pubsub::subscriber, public boost::enable_shared_from_this<subscriber>
		{
		public:
			stress_subscriber( pubsub::root& root, unsigned number_of_simulated_actions )
			  : root_(root)
			  , remaining_actions_(number_of_simulated_actions)
			{
			}

			void start()
			{
			}
		private:
			virtual void on_udate(const pubsub::node_name& name, const pubsub::node& data)
			{
			}

			virtual void on_invalid_node_subscription(const pubsub::node_name& node)
			{
			}

			virtual void on_unauthorized_node_subscription(const pubsub::node_name& node)
			{
			}

			virtual void on_failed_node_subscription(const pubsub::node_name& node)
			{
			}

			pubsub::root&	root_;
			unsigned		remaining_actions_;
		};

	}
}

#endif /* SIOUX_PUBSUB_STRESS_SUBSCRIBER_H */


