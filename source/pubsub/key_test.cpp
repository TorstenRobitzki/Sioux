// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include <boost/test/unit_test.hpp>
#include "pubsub/key.h"
#include "tools/test_order.h"

using namespace pubsub;
/**
 * @test strict weak order
 */
BOOST_AUTO_TEST_CASE(key_domain_order_test)
{
    std::vector<key_domain> values;
    values.push_back(key_domain("a"));
    values.push_back(key_domain("A"));
    values.push_back(key_domain("aa"));
    values.push_back(key_domain("12"));
    values.push_back(key_domain("foobar"));

    BOOST_CHECK( tools::check_weak_order( values.begin(), values.end() ) );
}
