// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "pubsub/test_helper.h"

namespace pubsub {
namespace test {
    /////////////////////
    // class subscriber
    subscriber::subscriber()
        : name_()
        , data_(node_version(), json::string("nearly random text."))
        , update_calls_(0)
    {
    }

    bool subscriber::notified(const node_name& name, const json::value& data)
    {
        const bool result = update_calls_ == 1 && name_ == name && data_.data() == data;
        update_calls_ = 0;

        return result;
    }

    bool subscriber::not_notified()
    {
        const bool result = update_calls_ == 0;
        update_calls_ = 0;

        return result;
    }

    void subscriber::on_udate(const node_name& name, const node& data)
    {
        ++update_calls_;
        name_ = name;
        data_ = data;
    }

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
        const boost::shared_ptr<authorization_call_back> cb = find_and_remove(authorization_request_, std::make_pair(user, name));

        if ( is_authorized )
        {
            cb->is_authorized();    
        }
        else
        {
            cb->not_authorized();
        }
    }

    void adapter::skip_authorization_request(const boost::shared_ptr<::pubsub::subscriber>& user, const node_name& name)
    {
        boost::mutex::scoped_lock lock(mutex_);
        find_and_remove(authorization_request_, std::make_pair(user, name));
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
        const boost::shared_ptr<validation_call_back> cb = find_and_remove(validation_request_, name);

        if ( is_valid )
        {
            cb->is_valid();
        }
        else
        {
            cb->not_valid();
        }
    }

    void adapter::skip_validation_request(const node_name& name)
    {
        boost::mutex::scoped_lock lock(mutex_);
        find_and_remove(validation_request_, name);
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
        find_and_remove(initialization_request_, name)->initial_value(answer);
    }

    void adapter::skip_initialization_request(const node_name& name)
    {
        boost::mutex::scoped_lock lock(mutex_);
        find_and_remove(initialization_request_, name);
    }

    bool adapter::empty() const
    {
        boost::mutex::scoped_lock lock(mutex_);
        return authorization_request_.empty() && validation_request_.empty() && initialization_request_.empty();
    }

    void adapter::valid_node(const node_name& name, const boost::shared_ptr<validation_call_back>& cb)
    {
        boost::mutex::scoped_lock lock(mutex_);
        validation_request_.insert(std::make_pair(name, cb));
    }

    void adapter::node_init(const node_name& name, const boost::shared_ptr<initialization_call_back>& cb)
    {
        boost::mutex::scoped_lock lock(mutex_);
        initialization_request_.insert(std::make_pair(name, cb));
    }

    void adapter::authorize(const boost::shared_ptr<::pubsub::subscriber>& user, const node_name& name, const boost::shared_ptr<authorization_call_back>& cb)
    {
        boost::mutex::scoped_lock lock(mutex_);
        const authorization_request_list::key_type key(user, name);
        authorization_request_.insert(std::make_pair(key, cb));
    }

}
}