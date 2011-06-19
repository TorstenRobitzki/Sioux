// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/proxy_connector.h"
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

BOOST_AUTO_TEST_CASE(get_set_proxy_configs)
{
    // there is a default c'tor
    server::proxy_configuration();

    const server::proxy_configuration config(
        server::proxy_configurator()
            .max_connections(15u)
            .max_idle_time(boost::posix_time::millisec(42))
            .connect_timeout(boost::posix_time::seconds(4)));

    BOOST_CHECK_EQUAL(15u, config.max_connections());
    BOOST_CHECK_EQUAL(boost::posix_time::millisec(42), config.max_idle_time());
    BOOST_CHECK_EQUAL(boost::posix_time::seconds(4), config.connect_timeout());
}
 
