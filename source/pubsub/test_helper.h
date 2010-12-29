// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_PUBSUB_TEST_HELPER_H
#define SIOUX_SOURCE_PUBSUB_TEST_HELPER_H

#include "pubsub/pubsub.h"
#include "pubsub/node.h"

namespace json {
    class value;
}

namespace pubsub {
namespace test {

    class subscriber : public ::pubsub::subscriber
    {
    public:
        subscriber();

        bool notified(const node_name&, const json::value&);
        bool not_notified();
    private:
        virtual void on_udate(const node_name& name, const node& data);

        node_name   name_;
        node        data_;
        unsigned    update_calls_;
    };

    class adapter : public ::pubsub::adapter
    {
    public:
        adapter();

        /**
         * @brief returns true, if the authorize() function was called exactly once 
         *        with the given parameters.
         */
        bool authorization_requested(const ::pubsub::subscriber&, const node_name&);
        bool validation_requested(const node_name&);
        bool initial_value_requested(const node_name&);
    private:
        virtual bool valid_node(const node_name& node_name);
        virtual json::value node_init(const node_name& node_name);
        virtual bool authorize(const ::pubsub::subscriber&, const node_name& node_name);

        unsigned                    authorize_calls_;
        const ::pubsub::subscriber* subscriber_;
        node_name                   authorized_node_name_;

        unsigned                    validation_calls_;
        node_name                   validated_node_name_;

        unsigned                    initialization_calls_;
        node_name                   initialization_node_name_;
    };
}
}
#endif // include guard


