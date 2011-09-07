// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_PUBSUB_STRESS_ADAPTER_H
#define SIOUX_PUBSUB_STRESS_ADAPTER_H

#include "pubsub/pubsub.h"
#include "pubsub/node.h"
#include <boost/asio/io_service.hpp>
#include <boost/thread/mutex.hpp>
#include <map>
#include <set>

namespace pubsub
{
	namespace test
	{
		/*!
		 * \brief adapter that decides whether a requested action succeeds or not, solely by the node name and a per subscriber
		 *        authorization
		 *
		 * For validation and initialization and authorization, there is are distinct key domains in the node name that
		 * describes if the requested action should succeed or not:
		 *
		 * "valid" : "\"valid\""         ; The node is valid. The callback will be called immediately.
		 *           "\"async_valid\""   ; The node is valid. The call to the callback will be postponed via the used io_service.
		 *           "\"async_invalid\"" ; The node is invalid. The call to the callback will be postponed via the used io_service.
		 *
		 * "init"  : "x" ; if the node_name contains a domain name "init" the node will not cause errors while
		 *                 initializing and return a value equal to json::parse("x").
		 *
		 * "async_init"  : "x" ; if the node_name contains a domain name "async_init" the node will not cause errors while
		 *                       initializing and will post a call to the initialization call back with x.
		 *
		 * "async_init_fail" :  ; If the node_name contains a domain "async_init_fail", the initialization failure will be postponed
		 *                        via the stored io_service.
		 *
		 * "async_auth" :       ; Answer to an authorization request will be posted to the given io_service.
		 */
		class stress_adapter : public ::pubsub::adapter
		{
		public:
			/*!
			 * \brief constructs a stress_adapter for testing and stores the passed io_service to be used to simulate
			 *        asynchronous behavior

			 * \param queue queue that is used to simulate deferred (asynchronous) answers to requests. The responding call
			 *              to a call back is posted to the queue if asynchronous response is used.
			 */
			explicit stress_adapter( boost::asio::io_service& queue );

			/*!
			 * \brief adds a new node to the subscribers list of authorized nodes.
			 *
			 * If the subscriber requests access to the given node, by subscribing to the node, the adapter will grand
			 * access, if the node name was previously added to the subscribers list of authorized nodes by calling this function.
			 * If the node name contains a key "auth"="\"async\"", the call to the given call back will be postponed.
			 *
			 * The subscribers address will be stored, to identify the subscriber later.
			 */
			void add_authorization(const subscriber& subcriber, const node_name& authorized_node);

			/*!
			 * \brief removes a node from the subscribers list of authorized nodes.
			 * \post the authorized_node must have been added to the subscribers list before.
			 */
			void remove_authorization(const subscriber& subcriber, const node_name& authorized_node);

		private:
			// no copy, no assigment
			stress_adapter(const stress_adapter&);
			stress_adapter& operator=(const stress_adapter&);

			// adapter implementation
			virtual void validate_node(const pubsub::node_name& node_name, const boost::shared_ptr<pubsub::validation_call_back>&);
			virtual void authorize(const boost::shared_ptr<pubsub::subscriber>&, const pubsub::node_name& node_name,
					const boost::shared_ptr<pubsub::authorization_call_back>&);
			virtual void node_init(const pubsub::node_name& node_name, const boost::shared_ptr<pubsub::initialization_call_back>&);

			typedef std::map< const void*, std::set<node_name> > authorized_nodes_by_subscriber_list_t;

			boost::mutex							mutex_;
			authorized_nodes_by_subscriber_list_t 	authorized_nodes_by_subscriber_;
			boost::asio::io_service&				io_queue_;
		};
	}
}

#endif /* SIOUX_PUBSUB_STRESS_ADAPTER_H */

