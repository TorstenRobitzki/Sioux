// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_PUBSUB_SUBSCRIBED_NODE_H
#define SIOUX_PUBSUB_SUBSCRIBED_NODE_H

#include "pubsub/node.h"
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <set>

namespace boost {
	namespace asio {
		class io_service;
	}
}

namespace pubsub
{
	class validation_call_back;
	class subscriber;
	class configuration;
	class adapter;

	namespace details {
		class node_validator;
		class user_authorizer;
		struct validation_step_data;
	}

	/**
	 * @brief class responsible for keeping track of a nodes data and subscriptions and a state concerning
	 *        the validity of the node and there subscriptions.
	 */
	class subscribed_node
	{
	public:
		/**
		 * @brief a new node starting with a state set to 'unvalidated'
		 */
		explicit subscribed_node(const boost::shared_ptr<const configuration>& config);

		/**
		 * @brief changes the data of the node.
		 *
		 * Depending on the current state of the node, subscribers will be informed about the changed data.
		 */
		void change_data(const node_name& name, const json::value& new_data);

		/**
		 * @brief adds a new subscriber to the list of subscribers or to the list of unauthorized subscribers.
		 *
		 * If the node is invalid, the passed subscriber will be informed synchronous and the passed adapter will be
		 * informed asynchronous.
		 */
		void add_subscriber(const boost::shared_ptr<subscriber>&, adapter&, boost::asio::io_service&);

		/**
		 * @brief removes the given subscriber from the list of authorized or unauthorized subscribers
		 */
		void remove_subscriber(const boost::shared_ptr<subscriber>&);

		/**
		 *  @brief marks this node as a valid node
		 */
		void validated(const details::node_validator&);

		/**
		 *  @brief mark this node as invalid node.
		 */
		void not_validated(const node_name& node_name);

		/**
		 * @brief confirm, that a subscriber was authorized to subscribe to the node
		 */
		void authorized_subscriber(const details::user_authorizer&);

		/**
		 * @brief the passed user is _not_ authorized to subscribe to this node
		 */
		void unauthorized_subscriber(const boost::shared_ptr<subscriber>& user);

		/**
		 * @brief initial data
		 */
		void initial_data(const node_name& name, const json::value& new_data);

		/**
		 * @brief the adapter failed to deliver initial data for this node
		 */
		void initial_data_failed(const node_name& name);
	private:
		void post_initialization_request(const details::validation_step_data&);

		typedef std::set<boost::shared_ptr<subscriber> > subscriber_list;

		boost::mutex							mutex_;
		node                                    data_;
		subscriber_list                         subscribers_;
		subscriber_list                         unauthorized_;

		enum {
			// possibly the node's name is not valid
			unvalidated,
			// it's clear, that this node is invalid or could not be initialized
			invalid,
			// nodes data is not initialized. Before the node gets initialized, at least one subscriber must be authorized.
			uninitialized,
			// initialization is requested, but not jet finished
			initializing,
			// node name is valid and node data contains valid data
			valid_and_initialized,
			// failed to get the initial data for this node
			initialization_failed
		} 										state_;
		boost::shared_ptr<const configuration>  config_;
	};

	/**
	 * @brief creates an initial validator implementation to start the process of validation a newly created
	 *        node with.
	 *
	 * @relates subscribed_node
	 */
	boost::shared_ptr<validation_call_back> create_validator(
			boost::shared_ptr<subscribed_node>& 	node,
			const node_name& 						node_name,
			const boost::shared_ptr<subscriber>&	user,
			boost::asio::io_service&				queue,
			adapter& 								adapter);

} // namespace pubsub

#endif // include guard

