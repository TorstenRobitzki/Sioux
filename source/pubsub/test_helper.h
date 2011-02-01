// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_PUBSUB_TEST_HELPER_H
#define SIOUX_SOURCE_PUBSUB_TEST_HELPER_H

#include "pubsub/pubsub.h"
#include "pubsub/node.h"
#include <map>
#include <boost/thread/mutex.hpp>

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

    /**
     * @brief implementation of the adapter interface for testing.
     * 
     */
    class adapter : public ::pubsub::adapter
    {
    public:
        /**
         * @brief returns true, if the authorize() function was called at least once 
         *        with the given parameters.
         */
        bool authorization_requested(const boost::shared_ptr<::pubsub::subscriber>&, const node_name&) const;
        void answer_authorization_request(const boost::shared_ptr<::pubsub::subscriber>&, const node_name&, bool is_authorized);
        void skip_authorization_request(const boost::shared_ptr<::pubsub::subscriber>&, const node_name&);

        bool validation_requested(const node_name&) const;
        void answer_validation_request(const node_name&, bool is_valid);
        void skip_validation_request(const node_name&);

        bool initialization_requested(const node_name&) const;
        void answer_initialization_request(const node_name&, const json::value& answer);
        void skip_initialization_request(const node_name&);

        /**
         * @brief no pending unanswered or unskiped requests
         */
        bool empty() const;
    private:
        virtual void valid_node(const node_name& node_name, const boost::shared_ptr<validation_call_back>&);
        virtual void authorize(const boost::shared_ptr<::pubsub::subscriber>&, const node_name& node_name, const boost::shared_ptr<authorization_call_back>&);
        virtual void node_init(const node_name& node_name, const boost::shared_ptr<initialization_call_back>&);

        typedef std::multimap<std::pair<const boost::shared_ptr<::pubsub::subscriber>, node_name>, boost::shared_ptr<authorization_call_back> > authorization_request_list;
        typedef std::multimap<node_name, boost::shared_ptr<validation_call_back> > validation_request_list;
        typedef std::multimap<node_name, boost::shared_ptr<initialization_call_back> > initialization_request_list;

        mutable boost::mutex        mutex_;
        authorization_request_list  authorization_request_;
        validation_request_list     validation_request_;
        initialization_request_list initialization_request_;
    };
}
}
#endif // include guard


