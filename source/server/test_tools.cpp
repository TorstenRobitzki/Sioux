// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/test_tools.h"
#include "tools/hexdump.h"
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>

void server::test::wait(const boost::posix_time::time_duration& period)
{
    boost::asio::io_service     queue;
    boost::asio::deadline_timer timer(queue, period);

    timer.wait();
}


