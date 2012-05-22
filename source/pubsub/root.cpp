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

        void subscribe(const boost::shared_ptr<subscriber>& s, const node_name& node_name)
        {
        	boost::shared_ptr<subscribed_node>			node;
        	boost::shared_ptr<validation_call_back>		validate;
        	boost::shared_ptr<authorization_call_back>	authorizer;

        	{
                boost::mutex::scoped_lock   lock(mutex_);
                const node_list_t::iterator pos = nodes_.find(node_name);

                if ( pos != nodes_.end() )
                {
                	node = pos->second;

                	if ( node->authorization_required() )
                		authorizer = create_authorizer( node, node_name, s, queue_, adapter_ );
                }
                else
                {
                	node.reset( new subscribed_node( configurations_.get_configuration( node_name ) ) );
                	validate = create_validator( node, node_name, s, queue_, adapter_ );
                	nodes_.insert( std::make_pair( node_name, node ) );
                }
            }

        	assert( node.get() );
        	node->add_subscriber(s, adapter_, queue_, node_name );

        	if ( validate.get() )
        	{
        		adapter_.validate_node( node_name, validate );
        	}
        	else if ( authorizer.get() )
        	{
        		adapter_.authorize( s, node_name, authorizer );
        	}
        }

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

        bool unsubscribe(const boost::shared_ptr<subscriber>& user, const node_name& node_name)
        {
            boost::mutex::scoped_lock   lock(mutex_);
            const node_list_t::iterator pos = nodes_.find(node_name);

            if ( pos != nodes_.end() )
            {
            	return pos->second->remove_subscriber(user);
            }

            return false;
        }

        unsigned unsubscribe_all( const boost::shared_ptr<subscriber>& user )
        {
            unsigned result = 0;

            boost::mutex::scoped_lock   lock(mutex_);

            for ( node_list_t::iterator node = nodes_.begin(), end = nodes_.end(); node != end; ++node )
            {
                if ( node->second->remove_subscriber( user ) )
                    ++result;
            }

            return result;
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

    void root::subscribe(const boost::shared_ptr<subscriber>& s, const node_name& node_name)
    {
        pimpl_->subscribe(s, node_name);
    }

    bool root::unsubscribe(const boost::shared_ptr<subscriber>& user, const node_name& node_name)
    {
    	return pimpl_->unsubscribe(user, node_name);
    }

    unsigned root::unsubscribe_all( const boost::shared_ptr<subscriber>& user )
    {
        return pimpl_->unsubscribe_all( user );
    }

    void root::update_node(const node_name& node_name, const json::value& new_data)
    {
        pimpl_->update_node(node_name, new_data);
    }

}
