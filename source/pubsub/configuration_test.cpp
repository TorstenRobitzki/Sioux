// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/unittest++.h"
#include "pubsub/configuration.h"
#include <boost/date_time/posix_time/posix_time.hpp> // required for i/o

using namespace pubsub;

/**
 * @brief test the default and setting and getting the node timeout parameter
 */
TEST(configure_node_timeout)
{
	CHECK_EQUAL(boost::posix_time::time_duration(), configuration().node_timeout());

	configuration config;
	CHECK_EQUAL(boost::posix_time::time_duration(), config.node_timeout());
	config.node_timeout(boost::posix_time::millisec(42));

	CHECK_EQUAL(boost::posix_time::millisec(42), config.node_timeout());
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
