// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/unittest++.h"
#include "pubsub/pubsub.h"
#include "pubsub/test_helper.h"

using namespace pubsub;

namespace {
    const node_name     random_node_name;
    const node          random_node_data(node_version(), json::number(12));
}

/**
 * @test test that the adapter 
 */
#if 0
TEST(root_subscribe_test)
{
    boost::asio::io_service queue;
    test::subscriber        user;
    test::adapter           adapter;
    pubsub::root            root(queue, adapter, boost::shared_ptr<const configuration>(new configuration));

    root.subscribe(user, random_node_name);

    CHECK(adapter.authorization_requested(user, random_node_name));
    CHECK(adapter.validation_requested(random_node_name));
    CHECK(adapter.initial_value_requested(random_node_name));
    CHECK(user.notified(random_node_name, random_node_data));
}
#endif 

