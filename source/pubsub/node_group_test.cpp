// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/unittest++.h"
#include "pubsub/node_group.h"

/**
 * @test check, that all node_names will pass a default constructed node_group
 */
TEST(default_node_group_test)
{
    pubsub::node_group  default_group;
    CHECK(default_group == pubsub::node_group());
    CHECK(pubsub::node_group() == default_group);
    CHECK(!(default_group != pubsub::node_group()));
}