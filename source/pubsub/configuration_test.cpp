// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>
#include "pubsub/configuration.h"
#include <boost/date_time/posix_time/posix_time.hpp> // required for i/o

using namespace pubsub;

/**
 * @brief test the default and setting and getting the node timeout parameter
 */
BOOST_AUTO_TEST_CASE(configure_node_timeout)
{
	BOOST_CHECK_EQUAL(boost::posix_time::time_duration(), configuration().node_timeout());

	configuration config;
	BOOST_CHECK_EQUAL(boost::posix_time::time_duration(), config.node_timeout());
	config.node_timeout(boost::posix_time::millisec(42));

	BOOST_CHECK_EQUAL(boost::posix_time::millisec(42), config.node_timeout());
}

BOOST_AUTO_TEST_CASE(configure_node_timeout_by_configurator)
{
	configuration config = configurator();

	BOOST_CHECK_EQUAL(boost::posix_time::time_duration(), config.node_timeout());

	config = configurator().node_timeout(boost::posix_time::millisec(42));

	BOOST_CHECK_EQUAL(boost::posix_time::millisec(42), config.node_timeout());
}

BOOST_AUTO_TEST_CASE(configure_authorization_required)
{
    configuration c1;
    // the default must be save
    BOOST_CHECK(c1.authorization_required());

    c1.authorization_required(false);
    BOOST_CHECK(!c1.authorization_required());

    c1.authorization_required(true);
    BOOST_CHECK(c1.authorization_required());
}

BOOST_AUTO_TEST_CASE(configure_authorization_required_by_configurator)
{
    configuration c1 = configurator();
    // the default must be save
    BOOST_CHECK(c1.authorization_required());

    c1 = configurator().authorization_not_required();
    BOOST_CHECK(!c1.authorization_required());

    c1 = configurator().authorization_required();
    BOOST_CHECK(c1.authorization_required());
}
