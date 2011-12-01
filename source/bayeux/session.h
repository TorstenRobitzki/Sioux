// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef BAYEUX_SESSION_H_
#define BAYEUX_SESSION_H_

#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/mutex.hpp>
#include <map>
#include <utility>

#include "pubsub/pubsub.h"
#include "pubsub/node.h"
#include "json/json.h"

namespace pubsub
{
    class root;
}

namespace bayeux
{
	class configuration;

	/**
	 * @brief interface of the asynchronous to the session, used by the session to inform the response about events
	 */
	class response_interface
	{
	public:
        virtual void second_connection_detected() = 0;
        virtual void new_messages( const json::array& ) = 0;
        virtual ~response_interface() {}
	};

	/**
	 * @brief this class holds the state of a client connected to a bayeux server.
	 *
	 * The purpose of the class is to connect a pubsub::node with the bayeux_response
	 * and thus connects to the http connection. A bayeux_response is only instantiated if a http
	 * request is currently pending. If the http request is currently not hold by the server, the
	 * session class will buffer data that have to be send to the bayeux client.
	 *
	 * To implement the requirement, that an id given by the publisher has to be published beside the published data,
	 * the pubsub data consists of an object with two field, one named 'data' and the other named 'id'.
	 *
	 * Responsibilities
	 * - buffer events from the pubsub::root
	 * - keeps the connection alive, by storing a reference to the bayeux implementation of the async_response
	 *   interface.
	 */
	class session : public pubsub::subscriber,
                    public boost::enable_shared_from_this< session >
	{
	public:
		/**
		 * @brief constructs a new session with the given session_id and configuration
		 *
		 * The session is stored, returned by session_id() and constant over the life time of the object.
		 * The configuration is used to limit the buffering of messages by number and total size.
		 */
		session( const std::string& session_id, pubsub::root& data,
		    const boost::shared_ptr< const configuration >& config );

		/**
		 * @brief returns the session id of this session.
		 *
		 * The session id is unique to a connector and will be valid as long, as this session
		 * is not removed from the connector.
		 */
		const json::string& session_id() const;

		/**
		 * @brief return the accumulated updates or stores the passed shared pointer to keep a reference until
		 *        an event occurs.
		 *
		 *        The accumulated updates are reset and a subsequence call to wait_for_event will yield an empty
		 *		  array.
		 */
		json::array wait_for_events( const boost::shared_ptr< response_interface >& response );

		/**
		 * @brief return the accumulated updates and resets the internal list
		 *
		 *        The accumulated updates are reset and a subsequence call to wait_for_event will yield an empty
		 *		  array. The function is nearly identically to wait_for_event() despite that this function will not
		 *		  hold the connection, but will return an empty array, if no events happened.
		 */
		json::array events();

		/**
		 * @brief starts a subscription request to the pubsub::node. Every subscriber-callback invoked with the
		 *        given node_name should be an response to this subscription
		 *
		 * @exception std::runtime_error if 'name' is empty
		 */
		void subscribe( const pubsub::node_name& name, const json::value* id );

		/**
		 * @brief unsubscribe from a node
		 */
        void unsubscribe( const pubsub::node_name& node, const json::value* id );
	private:
        // no copy, no assigment
        session( const session& );
        session& operator=( const session& );

        virtual void on_update(const pubsub::node_name& name, const pubsub::node& data);
        virtual void on_invalid_node_subscription(const pubsub::node_name& node);
        virtual void on_unauthorized_node_subscription(const pubsub::node_name& node);
        virtual void on_failed_node_subscription(const pubsub::node_name& node);

        struct subscription_context
        {
            json::value id_;
            bool        id_valid_;
        };

        json::object build_subscription_error_msg( const json::string& error_msg, const pubsub::node_name& node );
        json::object build_subscription_success_msg( const subscription_context& id, const pubsub::node_name& node ) const;
        json::object build_update_msg( const pubsub::node_name& name, const pubsub::node& data ) const;
        json::object build_unsubscribe_error_msg( const pubsub::node_name& node, const json::value* id ) const;
        json::object build_unsubscribe_success_msg( const pubsub::node_name& node, const json::value* id ) const;

        void add_subscription_id_if_exists( const pubsub::node_name& name, json::object& message );

        // adds the new message to the messages_ array and drops older messages from the front of the array until
        // the configured message size and count limit is reached.
        // The function expects the mutex_ to be locked
        void add_message_impl( const json::value& );
        void add_message_and_notify( const json::object& );
        void add_messages_and_notify( const json::array& );

        const json::string							session_id_;
        pubsub::root&                               root_;

        // used to synchronize the access to the members below
        boost::mutex								mutex_;

        json::array                                 messages_;
        boost::shared_ptr< response_interface >     http_connection_;
        boost::shared_ptr< const configuration >	config_;

        // used to synchronize the access to subscription_ids_
        boost::mutex                                subscription_mutex_;

        typedef std::multimap< pubsub::node_name, subscription_context > subscription_ids_t;
        subscription_ids_t                          subscription_ids_;
	};
}

#endif /* BAYEUX_SESSION_H_ */
