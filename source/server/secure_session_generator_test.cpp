// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include <boost/test/unit_test.hpp>
#include <set>
#include "server/secure_session_generator.h"

BOOST_AUTO_TEST_CASE( secure_session_generation_test )
{
    std::set< std::string >             session_ids;
    server::secure_session_generator    generator_impl;
    server::session_generator&          generator = generator_impl;

    static const int count = 1000000;

    for ( int i = 0; i != count; ++i )
    {
        session_ids.insert( generator("") );
    }

    BOOST_CHECK_GT( count * 10 / 9, session_ids.size() );
}
