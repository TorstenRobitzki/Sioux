// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/unittest++.h"
#include "pubsub/node.h"
#include "pubsub/root.h"
#include "pubsub/test_helper.h"
#include "pubsub/configuration.h"
#include <boost/asio/io_service.hpp>

using namespace pubsub;

namespace {
    const node_name     random_node_name;
    const json::number  random_node_data(12);
    const node          random_node(node_version(), json::number(12));

    // for unknown reason, a single call to poll_one() will not consume one posted action    
    void poll_queue(boost::asio::io_service& io)
    {
        for ( unsigned int i = 10; i; --i)
            io.poll_one();
    }

    test::subscriber& test_user(const boost::shared_ptr<::pubsub::subscriber>& u)
    {
        assert(u.get());
        return dynamic_cast<test::subscriber&>(*u.get());
    }

    /// checks that just validation was requested for the given name, not authorization was requested for the 
    /// given user and no initlization was requested. The given user was not notified.
    bool only_validation_requested(const test::adapter& adapter, const node_name& name, const boost::shared_ptr<::pubsub::subscriber>& user)
    {
        return adapter.validation_requested(name)
            && !adapter.authorization_requested(user, name)
            && !adapter.initialization_requested(name)
            && test_user(user).not_notified();
    }

    bool only_authorization_requested(const test::adapter& adapter, const node_name& name, const boost::shared_ptr<::pubsub::subscriber>& user)
    {
        return !adapter.validation_requested(name)
            && adapter.authorization_requested(user, name)
            && !adapter.initialization_requested(name)
            && test_user(user).not_notified();
    }

    bool only_initialization_requested(const test::adapter& adapter, const node_name& name, const boost::shared_ptr<::pubsub::subscriber>& user)
    {
        return !adapter.validation_requested(name)
            && !adapter.authorization_requested(user, name)
            && adapter.initialization_requested(name)
            && test_user(user).not_notified();
    }
}

/**
 * @test check, that a new node is validated, authorized and initialized in 
 *        exactly this order and not before the first step returned success.
 */
TEST(root_subscribe_test)
{
    boost::asio::io_service                 queue;
    test::adapter                           adapter;
    pubsub::root                            root(queue, adapter, configuration());

    boost::shared_ptr<::pubsub::subscriber> subscriber(new test::subscriber);

    root.subscribe(subscriber, random_node_name);
    poll_queue(queue);

    CHECK(only_validation_requested(adapter, random_node_name, subscriber));

    adapter.answer_validation_request(random_node_name, true);
    poll_queue(queue);

    CHECK(only_authorization_requested(adapter, random_node_name, subscriber));

    adapter.answer_authorization_request(subscriber, random_node_name, true);
    poll_queue(queue);

    CHECK(only_initialization_requested(adapter, random_node_name, subscriber));

    adapter.answer_initialization_request(random_node_name, random_node_data);
    poll_queue(queue);

    CHECK(adapter.empty());
    CHECK(test_user(subscriber).notified(random_node_name, random_node_data));
}

/**
 * @test check that a node that is configured to not require authorization,
 *       will not request authorization.
 */
TEST(subscribe_node_that_doesn_t_require_authorization)
{
    boost::asio::io_service                 queue;
    test::adapter                           adapter;
    pubsub::root                            root(queue, adapter, configurator().authorization_not_required());

    boost::shared_ptr<::pubsub::subscriber> subscriber(new test::subscriber);

    root.subscribe(subscriber, random_node_name);
    poll_queue(queue);

    CHECK(only_validation_requested(adapter, random_node_name, subscriber));

    adapter.answer_validation_request(random_node_name, true);
    poll_queue(queue);

    CHECK(only_initialization_requested(adapter, random_node_name, subscriber));

    adapter.answer_initialization_request(random_node_name, random_node_data);
    poll_queue(queue);

    CHECK(adapter.empty());
    CHECK(test_user(subscriber).notified(random_node_name, random_node_data));
}