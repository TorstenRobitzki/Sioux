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

    /////////////////
    // class adapter
    adapter::adapter()
        : authorize_calls_(0)
        , subscriber_(0)
        , authorized_node_name_()
        , validation_calls_(0)
        , validated_node_name_()
        , initialization_calls_(0)
        , initialization_node_name_()
    {
    }

    bool adapter::authorization_requested(const ::pubsub::subscriber& client, const node_name& name)
    {
        const bool result = authorize_calls_ == 1 && subscriber_ == &client && authorized_node_name_ == name;
        authorize_calls_ = 0;

        return result;
    }

    bool adapter::validation_requested(const node_name& node)
    {
        const bool result = validation_calls_ == 1 && validated_node_name_ == node;
        validation_calls_ = 0;

        return result;
    }

    bool adapter::initial_value_requested(const node_name& node)
    {
        const bool result = initialization_calls_ == 1 && initialization_node_name_ == node;
        initialization_calls_ = 0;

        return result;
    }

    bool adapter::valid_node(const node_name& node_name)
    {
        validated_node_name_ = node_name;
        ++validation_calls_;

        return true;
    }

    json::value adapter::node_init(const node_name& node_name)
    {
        initialization_node_name_ = node_name;
        ++initialization_calls_;

        return json::string();
    }

    bool adapter::authorize(const ::pubsub::subscriber& client, const node_name& node_name)
    {
        authorized_node_name_ = node_name;
        subscriber_ = &client;
        ++authorize_calls_;

        return true;
    }

}
}