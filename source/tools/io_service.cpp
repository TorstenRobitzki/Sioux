// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "tools/io_service.h"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

std::size_t tools::run(boost::asio::io_service& s)
{
    std::size_t sum = 0;
    std::size_t bridge_zero_loops = 2;

    // for unknown reasons, the run() function does not execute any element ins some occasions, even when one element was posted
    for ( ; bridge_zero_loops != 0; --bridge_zero_loops )
    {
        std::size_t now = s.run();

        if ( now != 0 )
        {
            bridge_zero_loops = 2;
        }
        else
        {
            s.reset();
        }

        sum += now;
    }

    return sum;
}

namespace {
    void run_pool_thread(boost::asio::io_service& q, std::size_t& result, boost::mutex& m )
    {
        const std::size_t c = tools::run(q);

        boost::mutex::scoped_lock lock(m);
        result += c;
    }
}

std::size_t tools::run(boost::asio::io_service& s, unsigned pool_size)
{
    std::size_t         result = 0;
    boost::mutex        mutex;
    boost::thread_group group;

    for ( ; pool_size; --pool_size )
        group.create_thread(boost::bind(&run_pool_thread, boost::ref(s), boost::ref(result), boost::ref(mutex)));

    group.join_all();

    return result;
}


