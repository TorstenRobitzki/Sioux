// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_PUBSUB_STRESS_SUBSCRIBER_H
#define SIOUX_PUBSUB_STRESS_SUBSCRIBER_H

#include "pubsub/pubsub.h"
#include "pubsub/root.h"
#include "pubsub/configuration.h"
#include "pubsub/node.h"

#include <boost/enable_shared_from_this.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/thread/mutex.hpp>

#include <set>

namespace pubsub
{
	class root;

	namespace test
	{
		class stress_adapter;

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
		class stress_subscriber : public pubsub::subscriber, public boost::enable_shared_from_this<stress_subscriber>
		{
		public:
			stress_subscriber( pubsub::root& root, pubsub::test::stress_adapter& adapter, unsigned number_of_simulated_actions, unsigned seed );

			/*!
			 * @brief starts acting like a simulated , stressing subscriber
			 */
			void start();

			/*!
			 * @brief throws an exception if there went something wrong during the test
			 */
			void check() const;

			struct subject;
		private:
			virtual void on_udate(const pubsub::node_name& name, const pubsub::node& data);
			virtual void on_invalid_node_subscription(const pubsub::node_name& node);
			virtual void on_unauthorized_node_subscription(const pubsub::node_name& node);
			virtual void on_failed_node_subscription(const pubsub::node_name& node);

			void next_action();
			void next_action_impl();
			void subscribe();
			void unsubscribe();
			void change();
			void change_configuration();

			boost::int32_t random( boost::int32_t lower, boost::int32_t upper );

			boost::mutex					mutex_;
			pubsub::root&					root_;
			unsigned						remaining_actions_;

	        typedef boost::uniform_int<std::size_t> rand_distribution_type;
	        typedef boost::variate_generator<boost::minstd_rand&, rand_distribution_type> rand_gen_type;

	        boost::minstd_rand              random_generator_;

			typedef std::set< const subject* > subjects_t;
			subjects_t						subscriptions_;
			subjects_t                      open_responses_;
			std::string						errors_;
		};

	}
}

#endif /* SIOUX_PUBSUB_STRESS_SUBSCRIBER_H */


