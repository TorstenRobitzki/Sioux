// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "pubsub/subscribed_node.h"
#include "pubsub/configuration.h"
#include "pubsub/pubsub.h"
#include "tools/scope_guard.h"
#include <boost/bind.hpp>

namespace pubsub {

	namespace details {

		// the data necessary for the actions along the chain validation->authorization->initialization
		struct validation_step_data
		{
			validation_step_data(
					boost::shared_ptr<subscribed_node>&		node,
					const node_name& 						node_name,
					boost::asio::io_service&				queue,
					adapter& 								adapter)
				: node_(node)
				, name_(node_name)
				, queue_(queue)
				, adapter_(adapter)
			{
			}

			boost::shared_ptr<subscribed_node>	node_;
			const node_name						name_;
			boost::asio::io_service&			queue_;
			adapter&							adapter_;
		};

		class node_validator : public validation_call_back, public validation_step_data
		{
		public:
			node_validator(boost::shared_ptr<subscribed_node>& 	node,
						   const node_name& 					node_name,
						   const boost::shared_ptr<subscriber>& user,
						   boost::asio::io_service&				queue,
						   adapter& 							adapter)
				: validation_step_data(node, node_name, queue, adapter)
				, user_(user)
				, commited_(false)
			{
			}

			~node_validator()
			{
				if ( !commited_ )
					not_valid();
			}

		private:
			void is_valid()
			{
				commited_ = true;
				node_->validated(*this);
			}

			void not_valid()
			{
				commited_ = true;
				node_->not_validated(name_);

				queue_.post(
					boost::bind(
						&adapter::invalid_node_subscription,
						&adapter_,
						name_,
						user_));
			}

			boost::shared_ptr<subscriber>	user_;
			bool							commited_;
		};

    	class user_authorizer : public authorization_call_back, public validation_step_data
		{
		public:
    		user_authorizer(const node_validator& previous_step, const boost::shared_ptr<subscriber>& user)
    			: validation_step_data(previous_step)
    			, user_(user)
    			, commited_(false)
			{
			}

    		user_authorizer(
    				boost::shared_ptr<subscribed_node>&		node,
					const boost::shared_ptr<subscriber>& 	user,
					const node_name& 						node_name,
					boost::asio::io_service&				queue,
					adapter& 								adapter	)
    			: validation_step_data(node, node_name, queue, adapter)
    			, user_(user)
    			, commited_(false)
    		{
    		}

    		~user_authorizer()
			{
				if ( !commited_ )
					not_authorized();
			}

    		boost::shared_ptr<subscriber> user_;
		private:
	        void is_authorized()
	        {
	        	commited_ = true;
	        	node_->authorized_subscriber(*this);
	        }

	        void not_authorized()
	        {
	        	commited_ = true;
	        	node_->unauthorized_subscriber(user_);

	        	user_->on_unauthorized_node_subscription(name_);
                queue_.post(
                    boost::bind(
                        &adapter::unauthorized_subscription,
                        &adapter_,
                        name_,
                        user_));
	        }

			bool commited_;
		};

    	class node_initializer : public initialization_call_back, public validation_step_data
    	{
    	public:
    		node_initializer(const validation_step_data& previous_step)
    			: validation_step_data(previous_step)
    			, commited_(false)
    		{
    		}

    		~node_initializer()
    		{
    			if ( !commited_ )
    			{
    				node_->initial_data_failed(name_);
                    queue_.post(
                        boost::bind(
                            &adapter::initialization_failed,
                            &adapter_,
                            name_));
    			}
    		}
    	private:
            void initial_value(const json::value& new_value)
            {
            	node_->initial_data(name_, new_value);
            	commited_ = true;
            }

			bool commited_;
    	};
	} // namespace details

	/////////////////////////
	// class subscribed_node
	subscribed_node::subscribed_node(const boost::shared_ptr<const configuration>& config)
		: mutex_()
		, data_(node_version(), json::null())
		, subscribers_()
		, unauthorized_()
		, state_( unvalidated )
		, config_( config )
	{
	}

	void subscribed_node::change_data(const node_name& name, const json::value& new_data)
	{
		boost::mutex::scoped_lock lock(mutex_);

		if ( !data_.update (new_data, config_->max_update_size() ) )
			return;

		if ( state_ == valid_and_initialized )
		{
			// notify all subscribed nodes
			for ( subscriber_list::const_iterator user = subscribers_.begin(); user != subscribers_.end(); ++user )
			{
				(*user)->on_update(name, data_);
			}
		}
	}

	void subscribed_node::add_subscriber( const boost::shared_ptr<subscriber>& user, adapter&, boost::asio::io_service&,
	    const node_name& name )
	{
		boost::mutex::scoped_lock lock(mutex_);

		if ( config_->authorization_required() )
		{
			unauthorized_.insert(user);
		}
		else
		{
			subscribers_.insert(user);

			if ( state_ == valid_and_initialized )
			    user->on_update( name, data_ );
		}
	}

	bool subscribed_node::remove_subscriber(const boost::shared_ptr<subscriber>& user)
	{
		boost::mutex::scoped_lock lock(mutex_);

		return subscribers_.erase(user) + unauthorized_.erase(user) != 0;
	}

	void subscribed_node::validated( const details::node_validator& last_step )
	{
		boost::mutex::scoped_lock lock(mutex_);
		assert(state_ == unvalidated);

		if ( config_->authorization_required() )
		{
			assert(subscribers_.empty());

			for ( subscriber_list::const_iterator user = unauthorized_.begin(); user != unauthorized_.end(); ++user )
			{
				boost::shared_ptr<details::user_authorizer> auth(new details::user_authorizer(last_step, *user));

				last_step.queue_.post(
					boost::bind(
						&adapter::authorize,
						&last_step.adapter_,
						boost::cref(auth->user_),
						boost::cref(auth->name_),
						boost::static_pointer_cast<authorization_call_back>(auth)));
			}

			state_ = uninitialized;
		}
		else
		{
			assert(unauthorized_.empty());

			post_initialization_request(last_step);

			state_ = initializing;
		}
	}

	void subscribed_node::not_validated(const node_name& node_name)
	{
		boost::mutex::scoped_lock lock(mutex_);
		assert(state_ == unvalidated);

		for ( subscriber_list::const_iterator user = subscribers_.begin(); user != subscribers_.end(); ++user )
			(*user)->on_invalid_node_subscription(node_name);

		subscribers_.clear();

		for ( subscriber_list::const_iterator user = unauthorized_.begin(); user != unauthorized_.end(); ++user )
			(*user)->on_invalid_node_subscription(node_name);

		unauthorized_.clear();

		state_ = invalid;
	}

    bool subscribed_node::authorization_required() const
    {
        return config_->authorization_required();
    }

	void subscribed_node::authorized_subscriber(const details::user_authorizer& auth)
	{
		boost::mutex::scoped_lock lock(mutex_);

		assert(state_ != unvalidated && state_ != invalid);

		const subscriber_list::iterator pos = std::find(unauthorized_.begin(), unauthorized_.end(), auth.user_);

		if ( pos == unauthorized_.end() )
			return;

		unauthorized_.erase(pos);
		subscribers_.insert(auth.user_);

		if ( state_ == uninitialized )
		{
			post_initialization_request(auth);
		}
		else if ( state_ == valid_and_initialized )
		{
			auth.user_->on_update(auth.name_, data_);
		}
	}

	void subscribed_node::unauthorized_subscriber(const boost::shared_ptr<subscriber>& user)
	{
		boost::mutex::scoped_lock lock(mutex_);

		const subscriber_list::iterator pos = std::find(unauthorized_.begin(), unauthorized_.end(), user);

		if ( pos != unauthorized_.end() )
			unauthorized_.erase(user);
	}

	void subscribed_node::initial_data(const node_name& name, const json::value& new_data)
	{
		boost::mutex::scoped_lock lock(mutex_);
		assert(state_ = initializing);
		state_ = valid_and_initialized;

		data_.update(new_data, config_->max_update_size());

		for ( subscriber_list::iterator subscriber = subscribers_.begin(); subscriber != subscribers_.end(); ++subscriber )
		{
			(*subscriber)->on_update(name, data_);
		}
	}

	void subscribed_node::initial_data_failed(const node_name& name)
	{
		boost::mutex::scoped_lock lock(mutex_);
		assert(state_ = initializing);
		state_ = initialization_failed;

		tools::scope_guard clear_subscribers  = tools::make_obj_guard(subscribers_, &subscriber_list::clear);
		static_cast<void>(clear_subscribers);
		tools::scope_guard clear_unauthorized = tools::make_obj_guard(unauthorized_, &subscriber_list::clear);
		static_cast<void>(clear_unauthorized);

		for ( subscriber_list::iterator subscriber = subscribers_.begin(); subscriber != subscribers_.end(); ++subscriber )
		{
			(*subscriber)->on_failed_node_subscription(name);
		}

		for ( subscriber_list::iterator subscriber = unauthorized_.begin(); subscriber != unauthorized_.end(); ++subscriber )
		{
			(*subscriber)->on_failed_node_subscription(name);
		}
	}

	void subscribed_node::post_initialization_request(const details::validation_step_data& last_step)
	{
		state_ = initializing;
		const boost::shared_ptr<details::node_initializer> init(new details::node_initializer(last_step));

		last_step.queue_.post(
			boost::bind(
				&adapter::node_init,
				&last_step.adapter_,
				boost::cref(init->name_),
				boost::static_pointer_cast<initialization_call_back>(init)));

	}

	boost::shared_ptr<validation_call_back> create_validator(
			boost::shared_ptr<subscribed_node>& 	node,
			const node_name& 						node_name,
			const boost::shared_ptr<subscriber>&	user,
			boost::asio::io_service&				queue,
			adapter& 								adapter)
	{
		return boost::shared_ptr<validation_call_back>(new details::node_validator(node, node_name, user, queue, adapter));
	}

	boost::shared_ptr<authorization_call_back> create_authorizer(
			boost::shared_ptr<subscribed_node>& 	node,
			const node_name& 						node_name,
			const boost::shared_ptr<subscriber>&	user,
			boost::asio::io_service&				queue,
			adapter& 								adapter)
	{
		return boost::shared_ptr<authorization_call_back>(new details::user_authorizer(node, user, node_name, queue, adapter));
	}
}

