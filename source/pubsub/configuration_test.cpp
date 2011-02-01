// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/unittest++.h"
#include "pubsub/configuration.h"

using namespace pubsub;

TEST(configure_node_timeout)
{
}

TEST(configure_node_timeout_by_configurator)
{
}

TEST(configure_authorization_required)
{
    configuration c1;
    // the default must be save
    CHECK(c1.authorization_required());

    c1.authorization_required(false);
    CHECK(!c1.authorization_required());

    c1.authorization_required(true);
    CHECK(c1.authorization_required());
}

TEST(configure_authorization_required_by_configurator)
{
    configuration c1 = configurator();
    // the default must be save
    CHECK(c1.authorization_required());

    c1 = configurator().authorization_not_required();
    CHECK(!c1.authorization_required());

    c1 = configurator().authorization_required();
    CHECK(c1.authorization_required());
}
