// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "pubsub/root.h"
#include "pubsub/pubsub.h"
#include "pubsub/configuration.h"
#include "pubsub/node_group.h"
#include "pubsub/node.h"
#include "pubsub/subscribed_node.h"
#include "tools/asstring.h"
#include <vector>
#include <map>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>

namespace pubsub {
namespace {
    class configuration_list
    {
    public:
        explicit configuration_list(const configuration& default_configuration)
            : default_(new configuration(default_configuration))
        {
        }

        void add_configuration(const node_group& node_name, const configuration& new_config)
        {
            const list_t::value_type new_value(node_name, boost::shared_ptr<const configuration>(new configuration(new_config)));
            configurations_.push_back(new_value);
        }

        void remove_configuration(const node_group& node_name)
        {
            list_t::iterator pos = configurations_.begin();

            for ( ; pos != configurations_.end() && pos->first == node_name; ++pos )
                ;

            if ( pos == configurations_.end() )
                throw std::runtime_error("no such configuration: " + tools::as_string(node_name));

            configurations_.erase(pos);
        }

        boost::shared_ptr<const configuration> get_configuration(const node_name& name) const
        {
            list_t::const_iterator pos = configurations_.begin();

            for ( ; pos != configurations_.end() && !pos->first.in_group(name); ++pos )
                ;

            return pos == configurations_.end()
                ? default_
                : pos->second;
        }

    private:
        typedef std::vector<std::pair<node_group, boost::shared_ptr<const configuration> > > list_t;
        list_t                                          configurations_;
        const boost::shared_ptr<const configuration>    default_;
    };
}

    namespace {
        template <class Root>
        class validator
            : public boost::enable_shared_from_this<validator<Root> >
            , public validation_call_back
            , public initialization_call_back
            , public authorization_call_back
        {
        public:
            validator(boost::asio::io_service& io_queue, adapter& a, const node_name& node, const boost::shared_ptr<subscriber>& usr, const boost::shared_ptr<const configuration>& config, Root& root)
                : adapter_(a)
                , answered_(false)
                , node_(node)
                , user_(usr)
                , config_(config)
                , queue_(io_queue)
                , destructor_action_(report_invalid_node)
                , root_impl_(root)
            {
            }

            virtual ~validator()
            {
                try {
                    switch ( destructor_action_ )
                    {
                    case report_invalid_node:
                        not_valid();
                        break;
                    case report_unauthorized_subscription:
                        not_authorized();
                        break;
                    case report_initialization_failed:
                        not_initialized();
                        break;
                    case report_nothing:
                    	break;
                    }
                }
                catch (...)
                {
                }
            }
        private:
            void post_initilization_request()
            {
                queue_.post(
                    boost::bind(
                        &adapter::node_init, 
                        boost::ref(adapter_), 
                        boost::cref(node_), 
                        this->shared_from_this()));

                destructor_action_ = report_initialization_failed;
            }

            void is_valid() 
            {
                if ( config_->authorization_required() )
                {
                    queue_.post(
                        boost::bind(
                            &adapter::authorize, 
                            boost::ref(adapter_), 
                            boost::cref(user_),
                            boost::cref(node_), 
                            this->shared_from_this()));

                    destructor_action_ = report_unauthorized_subscription;
                }
                else
                {
                    post_initilization_request();
                }
            }

            void not_valid() 
            {
                user_->on_invalid_node_subscription(node_);
                queue_.post(
                    boost::bind(
                        &adapter::invalid_node_subscription,
                        &adapter_,
                        node_,
                        user_));

                destructor_action_ = report_nothing;
            }

            void is_authorized()
            {
                post_initilization_request();
            }

            void not_authorized()
            {
                user_->on_unauthorized_node_subscription(node_);
                queue_.post(
                    boost::bind(
                        &adapter::unauthorized_subscription,
                        &adapter_,
                        node_,
                        user_));

                destructor_action_ = report_nothing;
            }

            void initial_value(const json::value& data)
            {
                // Finally, an initialized, valid and authorized node
                root_impl_.add_initial_authorized_and_validated_node(node_, data, user_, config_);
                destructor_action_ = report_nothing;
            }

            void not_initialized()
            {
                user_->on_failed_node_subscription(node_);

                queue_.post(
                    boost::bind(
                    &adapter::initialization_failed,    
                    &adapter_,
                    node_,
                    user_));
            }

            adapter&                                        adapter_;
            bool                                            answered_;
            const node_name                                 node_;
            const boost::shared_ptr<subscriber>             user_;
            const boost::shared_ptr<const configuration>    config_;
            boost::asio::io_service&                        queue_;

            enum {
                report_invalid_node,
                report_unauthorized_subscription,
                report_initialization_failed,
                report_nothing
            }                                               destructor_action_;

            Root&                                           root_impl_;
        };
    }



    class root::impl
    {
    public:
        impl(boost::asio::io_service& io_queue, adapter& adapter, const configuration& default_configuration)
            : queue_(io_queue)
            , adapter_(adapter)
            , configurations_(default_configuration)
        {
        }

        void add_configuration(const node_group& node_name, const configuration& new_config)
        {
            boost::mutex::scoped_lock   lock(mutex_);
            configurations_.add_configuration(node_name, new_config);
        }

        void remove_configuration(const node_group& node_name)
        {
            boost::mutex::scoped_lock   lock(mutex_);
            configurations_.remove_configuration(node_name);
        }

        void subscribe(boost::shared_ptr<subscriber>& s, const node_name& node_name)
        {
        	boost::shared_ptr<subscribed_node>		node;
        	boost::shared_ptr<validation_call_back>	validate;

        	{
                boost::mutex::scoped_lock   lock(mutex_);
                const node_list_t::iterator pos = nodes_.find(node_name);

                if ( pos != nodes_.end() )
                {
                	node = pos->second;
                }
                else
                {
                	node.reset(new subscribed_node(configurations_.get_configuration(node_name)));
                	validate = create_validator(node, node_name, s, queue_, adapter_);
                	nodes_.insert(std::make_pair(node_name, node));
                }
            }

        	assert(node.get());
        	node->add_subscriber(s, adapter_, queue_);

        	if ( validate.get() )
        	{
        		adapter_.validate_node(node_name, validate);
        	}
        }

        /*
        void add_initial_authorized_and_validated_node(const node_name& node, const json::value& data, const boost::shared_ptr<subscriber>& usr, const boost::shared_ptr<const configuration>& config)
        {

        	boost::mutex::scoped_lock   lock(mutex_);
            const node_list_t::iterator pos = nodes_.find(node);

            if ( pos != nodes_.end() )
            {                

            }
            else
            {
                boost::shared_ptr<subscribed_node> new_node(new subscribed_node(usr, data, config));
                nodes_.insert(std::make_pair(node, new_node));

                new_node->update_all_subscribers(node, queue_);
            }
        }
*/
        void update_node(const node_name& node_name, const json::value& new_data)
        {
        	boost::shared_ptr<subscribed_node>		node;

        	{
				boost::mutex::scoped_lock   lock(mutex_);
				const node_list_t::iterator pos = nodes_.find(node_name);

				if ( pos != nodes_.end() )
					node = pos->second;
        	}

        	if ( node.get() )
        	{
        		node->change_data(node_name, new_data);
			}
        }

        void unsubscribe(const boost::shared_ptr<subscriber>& user, const node_name& node_name)
        {
            boost::mutex::scoped_lock   lock(mutex_);
            const node_list_t::iterator pos = nodes_.find(node_name);

            if ( pos != nodes_.end() )
            {
            	pos->second->remove_subscriber(user);
            }
        }
    private:
        boost::asio::io_service&                queue_;
        adapter&                                adapter_;

        boost::mutex                            mutex_;
        configuration_list                      configurations_;
        typedef std::map<node_name, boost::shared_ptr<subscribed_node> > node_list_t;
        node_list_t                             nodes_;
    };

    root::root(boost::asio::io_service& io_queue, adapter& adapter, const configuration& default_configuration)
        : pimpl_(new impl(io_queue, adapter, default_configuration))
    {
    }

    void root::add_configuration(const node_group& node_name, const configuration& new_config)
    {
        pimpl_->add_configuration(node_name, new_config);
    }

    void root::remove_configuration(const node_group& node_name)
    {
        pimpl_->remove_configuration(node_name);
    }

    void root::subscribe(boost::shared_ptr<subscriber>& s, const node_name& node_name)
    {
        pimpl_->subscribe(s, node_name);
    }

    void root::unsubscribe(const boost::shared_ptr<subscriber>& user, const node_name& node_name)
    {
    	pimpl_->unsubscribe(user, node_name);
    }

    void root::unsubscribe_all(const boost::shared_ptr<subscriber>&)
    {
    }

    void root::update_node(const node_name& node_name, const json::value& new_data)
    {
    	pimpl_->update_node(node_name, new_data);
    }

}
