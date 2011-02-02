// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "pubsub/test_helper.h"
#include <utility>

namespace pubsub {
namespace test {
    namespace {
        template <class Con>
        typename Con::mapped_type find_and_remove(Con& container, const typename Con::key_type& key)
        {
            const typename Con::iterator pos = container.find(key);

            if ( pos == container.end() )
                throw std::runtime_error("can't find key");

            const Con::mapped_type result = pos->second;
            container.erase(pos);

            return result;
        }

        template <class Con, class Key>
        bool search_and_remove(Con& container, const Key& key)
        {
            const typename Con::iterator pos = container.find(key);

            const bool result = pos != container.end();

            if ( result )
                container.erase(pos);

            return result;
        }

        template <class Con, class Key>
        bool search_and_remove(Con& container, const Key& key, typename Con::mapped_type& deleted)
        {
            const typename Con::iterator pos = container.find(key);

            const bool result = pos != container.end();

            if ( result )
            {
                deleted = pos->second;
                container.erase(pos);
            }

            return result;
        }
    }

    /////////////////////
    // class subscriber
    bool subscriber::on_udate_called(const node_name& n, const json::value& v)
    {
        boost::mutex::scoped_lock lock(mutex_);
        return search_and_remove(on_udate_calls_, boost::make_tuple(n,v));
    }

    bool subscriber::not_on_udate_called() const
    {
        boost::mutex::scoped_lock lock(mutex_);
        return on_udate_calls_.empty();
    }

    bool subscriber::on_invalid_node_subscription_called(const node_name& node)
    {
        boost::mutex::scoped_lock lock(mutex_);
        return search_and_remove(on_invalid_node_subscription_calls_, node);
    }

    bool subscriber::not_on_invalid_node_subscription_called() const
    {
        boost::mutex::scoped_lock lock(mutex_);
        return on_invalid_node_subscription_calls_.empty();
    }

    bool subscriber::on_unauthorized_node_subscription_called(const node_name& node)
    {
        boost::mutex::scoped_lock lock(mutex_);
        return search_and_remove(on_unauthorized_node_subscription_calls_, node);
    }

    bool subscriber::not_on_unauthorized_node_subscription_called() const
    {
        boost::mutex::scoped_lock lock(mutex_);
        return on_unauthorized_node_subscription_calls_.empty();
    }

    bool subscriber::on_failed_node_subscription_called(const node_name& node)
    {
        boost::mutex::scoped_lock lock(mutex_);
        return search_and_remove(on_failed_node_subscription_calls_, node);
    }

    bool subscriber::not_on_failed_node_subscription_called() const
    {
        boost::mutex::scoped_lock lock(mutex_);
        return on_failed_node_subscription_calls_.empty();
    }

    bool subscriber::empty() const
    {
        boost::mutex::scoped_lock lock(mutex_);
        return on_udate_calls_.empty() 
            && on_invalid_node_subscription_calls_.empty()
            && on_unauthorized_node_subscription_calls_.empty()
            && on_failed_node_subscription_calls_.empty();
    }

    void subscriber::on_udate(const node_name& name, const node& data)
    {
        boost::mutex::scoped_lock lock(mutex_);
        on_udate_calls_.insert(boost::make_tuple(name, data.data()));
    }

    void subscriber::on_invalid_node_subscription(const node_name& node)
    {
        boost::mutex::scoped_lock lock(mutex_);
        on_invalid_node_subscription_calls_.insert(node);
    }

    void subscriber::on_unauthorized_node_subscription(const node_name& node)
    {
        boost::mutex::scoped_lock lock(mutex_);
        on_unauthorized_node_subscription_calls_.insert(node);
    }

    void subscriber::on_failed_node_subscription(const node_name& node)
    {
        boost::mutex::scoped_lock lock(mutex_);
        on_failed_node_subscription_calls_.insert(node);
    }

    /////////////////
    // class adapter
    bool adapter::authorization_requested(const boost::shared_ptr<::pubsub::subscriber>& user, const node_name& name) const
    {
        boost::mutex::scoped_lock lock(mutex_);
        const authorization_request_list::const_iterator pos = authorization_request_.find(std::make_pair(user, name));

        return pos != authorization_request_.end();
    }

    void adapter::answer_authorization_request(const boost::shared_ptr<::pubsub::subscriber>& user, const node_name& name, bool is_authorized)
    {
        boost::mutex::scoped_lock lock(mutex_);

        boost::shared_ptr<authorization_call_back> cb;        
        if ( search_and_remove(authorization_request_, std::make_pair(user, name), cb) )
        {
            is_authorized ? cb->is_authorized() : cb->not_authorized();
        }
        else
        {
            authorization_answers_.insert(std::make_pair(std::make_pair(user, name), is_authorized));
        }
    }

    void adapter::skip_authorization_request(const boost::shared_ptr<::pubsub::subscriber>& user, const node_name& name)
    {
        boost::mutex::scoped_lock lock(mutex_);

        if ( !search_and_remove(authorization_request_, std::make_pair(user, name)) )
        {
            authorizations_to_skip_.insert(std::make_pair(user, name));
        }
    }

    bool adapter::validation_requested(const node_name& name) const
    {
        boost::mutex::scoped_lock lock(mutex_);
        const validation_request_list::const_iterator pos = validation_request_.find(name);

        return pos != validation_request_.end();
    }

    void adapter::answer_validation_request(const node_name& name, bool is_valid)
    {
        boost::mutex::scoped_lock lock(mutex_);
        
        boost::shared_ptr<validation_call_back> cb;
        if ( search_and_remove(validation_request_, name, cb) )
        {
            is_valid ? cb->is_valid() : cb->not_valid();
        }
        else
        {
            validation_answers_.insert(std::make_pair(name, is_valid));
        }
    }

    void adapter::skip_validation_request(const node_name& name)
    {
        boost::mutex::scoped_lock lock(mutex_);

        if ( !search_and_remove(validation_request_, name) )
        {
            validations_to_skip_.insert(name);
        }
    }

    bool adapter::initialization_requested(const node_name& name) const
    {
        boost::mutex::scoped_lock lock(mutex_);
        const initialization_request_list::const_iterator pos = initialization_request_.find(name);

        return pos != initialization_request_.end();
    }

    void adapter::answer_initialization_request(const node_name& name, const json::value& answer)
    {
        boost::mutex::scoped_lock lock(mutex_);

        boost::shared_ptr<initialization_call_back> cb;
        if ( search_and_remove(initialization_request_, name, cb) )
        {
            cb->initial_value(answer);
        }
        else
        {
            initialization_answers_.insert(std::make_pair(name, answer));
        }
    }

    void adapter::skip_initialization_request(const node_name& name)
    {
        boost::mutex::scoped_lock lock(mutex_);
        if ( !search_and_remove(initialization_request_, name) )
        {
            initializations_to_skip_.insert(name);
        }
    }

    bool adapter::invalid_node_subscription_reported(const node_name& node, const boost::shared_ptr<::pubsub::subscriber>& user)
    {
        return search_and_remove(invalid_node_subscription_reports_, boost::make_tuple(node, user));
    }

    bool adapter::unauthorized_subscription_reported(const node_name& node, const boost::shared_ptr<::pubsub::subscriber>& user)
    {
        return search_and_remove(unauthorized_subscription_reports_, boost::make_tuple(node, user));
    }

    bool adapter::initialization_failed_reported(const node_name& node, const boost::shared_ptr<::pubsub::subscriber>& user)
    {
        return search_and_remove(initialization_failed_reports_, boost::make_tuple(node, user));
    }

    bool adapter::empty() const
    {
        boost::mutex::scoped_lock lock(mutex_);
        return authorization_request_.empty() && validation_request_.empty() && initialization_request_.empty()
            && invalid_node_subscription_reports_.empty() && unauthorized_subscription_reports_.empty() && initialization_failed_reports_.empty()
            && authorization_answers_.empty() && validation_answers_.empty() && initialization_answers_.empty()
            && validations_to_skip_.empty() && authorizations_to_skip_.empty() && initializations_to_skip_.empty();
    }

    void adapter::valid_node(const node_name& name, const boost::shared_ptr<validation_call_back>& cb)
    {
        boost::mutex::scoped_lock lock(mutex_);

        bool answer = false;
        
        if ( search_and_remove(validation_answers_, name, answer) )
        {
            answer ? cb->is_valid() : cb->not_valid();
        }
        else if ( !search_and_remove(validations_to_skip_, name ) )
        {
            validation_request_.insert(std::make_pair(name, cb));
        }
    }

    void adapter::node_init(const node_name& name, const boost::shared_ptr<initialization_call_back>& cb)
    {
        boost::mutex::scoped_lock lock(mutex_);

        json::value value = json::number(42);
        if ( search_and_remove(initialization_answers_, name, value) )
        {
            cb->initial_value(value);
        }
        else if ( !search_and_remove(initializations_to_skip_, name) )
        {
            initialization_request_.insert(std::make_pair(name, cb));
        }
    }

    void adapter::authorize(const boost::shared_ptr<::pubsub::subscriber>& user, const node_name& name, const boost::shared_ptr<authorization_call_back>& cb)
    {
        boost::mutex::scoped_lock lock(mutex_);
        const authorization_request_list::key_type key(user, name);

        bool answer = false;
        if ( search_and_remove(authorization_answers_, key, answer) )
        {
            answer ? cb->is_authorized() : cb->not_authorized();
        }
        else if ( !search_and_remove(authorizations_to_skip_, key) )
        {
            authorization_request_.insert(std::make_pair(key, cb));
        }
    }

    void adapter::invalid_node_subscription(const node_name& node, const boost::shared_ptr<::pubsub::subscriber>& user)
    {
        invalid_node_subscription_reports_.insert(boost::make_tuple(node, user));
    }

    void adapter::unauthorized_subscription(const node_name& node, const boost::shared_ptr<::pubsub::subscriber>& user)
    {
        unauthorized_subscription_reports_.insert(boost::make_tuple(node, user));
    }

    void adapter::initialization_failed(const node_name& node, const boost::shared_ptr<::pubsub::subscriber>& user)
    {
        initialization_failed_reports_.insert(boost::make_tuple(node, user));
    }

}
}