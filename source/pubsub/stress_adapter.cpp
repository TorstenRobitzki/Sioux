// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "pubsub/stress_adapter.h"
#include <boost/bind.hpp>
#include <cassert>

namespace pubsub
{
	namespace test
	{
		stress_adapter::stress_adapter( boost::asio::io_service& queue )
			: mutex_()
			, authorized_nodes_by_subscriber_()
			, io_queue_( queue )
		{
		}

		void stress_adapter::add_authorization(const subscriber& user, const node_name& authorized_node)
		{
            boost::mutex::scoped_lock   lock(mutex_);
            const void* user_id = &user;
            authorized_nodes_by_subscriber_[user_id].insert(authorized_node);
		}

		void stress_adapter::remove_authorization(const subscriber& usr, const node_name& authorized_node)
		{
            boost::mutex::scoped_lock   lock(mutex_);
            authorized_nodes_by_subscriber_list_t::iterator pos = authorized_nodes_by_subscriber_.find(&usr);

            // the user must be known already and the node must have been authorized before
            assert( pos != authorized_nodes_by_subscriber_.end() );
            assert(	pos->second.find(authorized_node) != pos->second.end() );

            pos->second.erase(authorized_node);
            if ( pos->second.empty() )
            	authorized_nodes_by_subscriber_.erase(pos);
		}

		void stress_adapter::validate_node(const pubsub::node_name& node_name, const boost::shared_ptr<pubsub::validation_call_back>& cb)
		{
			const std::pair<bool, key> key = node_name.find_key(key_domain("valid"));

			if ( key.first && key.second.value() == "valid" )
			{
				cb->is_valid();
			}
			else if ( key.first && key.second.value() == "async_valid" )
			{
				io_queue_.post(boost::bind(&pubsub::validation_call_back::is_valid, cb));
			}
			else if ( key.first && key.second.value() == "async_invalid" )
			{
				io_queue_.post(boost::bind(&pubsub::validation_call_back::not_valid, cb));
			}
			else
			{
				cb->not_valid();
			}
		}

		void stress_adapter::authorize(const boost::shared_ptr<pubsub::subscriber>& usr, const pubsub::node_name& node_name,
				const boost::shared_ptr<pubsub::authorization_call_back>& cb)
		{
			bool asynchronous = node_name.find_key(key_domain("async_auth")).first;

			boost::mutex::scoped_lock   lock(mutex_);
            authorized_nodes_by_subscriber_list_t::const_iterator pos = authorized_nodes_by_subscriber_.find(usr.get());

            if ( pos != authorized_nodes_by_subscriber_.end() && pos->second.find(node_name) != pos->second.end() )
            {
            	if ( asynchronous )
            	{
            		io_queue_.post(boost::bind(&pubsub::authorization_call_back::is_authorized, cb));
            	}
            	else
            	{
            		cb->is_authorized();
            	}
            }
		}

		// used, to store an instance of initialization_call_back, in the io_service, to defer the destruction of the call back.
		static void defer_initialization_failure(const boost::shared_ptr<pubsub::initialization_call_back>&)
		{
		}

		void stress_adapter::node_init(const pubsub::node_name& node_name, const boost::shared_ptr<pubsub::initialization_call_back>& cb)
		{
			const std::pair<bool, key> sync_key = node_name.find_key(key_domain("init"));

			if ( sync_key.first )
			{
				cb->initial_value(json::parse(sync_key.second.value()));
				return;
			}

			const std::pair<bool, key> async_key = node_name.find_key(key_domain("async_init"));

			if ( async_key.first )
			{
				const json::value initial_value = json::parse(async_key.second.value());
				io_queue_.post( boost::bind(&pubsub::initialization_call_back::initial_value, cb, initial_value));
				return;
			}

			const std::pair<bool, key> async_fail = node_name.find_key(key_domain("async_init_fail"));

			if ( async_fail.first )
				io_queue_.post(boost::bind(&defer_initialization_failure, cb));
		}
	}
}
