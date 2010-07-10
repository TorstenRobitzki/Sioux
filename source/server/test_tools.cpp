// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/test_tools.h"

std::size_t server::test::run(boost::asio::io_service& s)
{
    std::size_t sum = 0;
    std::size_t bridge_zero_loops = 2;

    // for unknown reasons, the run() function does not execute any element ins some occasions, even when one element was posted
    for ( ; bridge_zero_loops != 0; --bridge_zero_loops )
    {
        std::size_t now = s.run();

        if ( now != 0 )
            bridge_zero_loops = 2;

        sum += now;
    }

    return sum;
}

