// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include <boost/test/unit_test.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace {
    struct handler
    {
        void operator()(boost::system::error_code)
        {
            called = true;
            time   = boost::posix_time::microsec_clock::universal_time();
        }

        bool&                       called;
        boost::posix_time::ptime&   time;
    };

    struct start_timer
    {
        boost::asio::deadline_timer&    timer;
        handler&                        h;

        void operator()()
        {
            timer.expires_from_now(boost::posix_time::seconds(1));
            timer.async_wait(h);
        }
    };
}

/** 
 * @test this test is intended to check that io_service::run() returns when only a aync_wait is scheduled
 */
BOOST_AUTO_TEST_CASE(wait_lasts_time)
{
    boost::asio::io_service     queue;
    boost::asio::deadline_timer timer(queue);
    bool                        called = false;
    boost::posix_time::ptime    time;
    boost::posix_time::ptime    now = boost::posix_time::microsec_clock::universal_time();

    handler                     h = { called, time };
    start_timer                 s = { timer, h };

    queue.post(s);

    BOOST_CHECK_EQUAL(2u, queue.run());
    BOOST_CHECK_GE(time - now, boost::posix_time::seconds(1) - boost::posix_time::millisec(100));
    BOOST_CHECK_LE(time - now, boost::posix_time::seconds(1) + boost::posix_time::millisec(100));
    BOOST_CHECK(called);
}
