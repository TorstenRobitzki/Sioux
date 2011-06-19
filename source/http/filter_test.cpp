// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>
#include "http/filter.h"
#include <cstring>

namespace {
    // used to convert string literals to tools::substring, just for test purpose
    tools::substring _(const char* str)
    {
        return tools::substring(str, std::strlen(str) + str);
    }
}

/** 
 * @test contruct filter with different version of upper lower letters etc.
 */
BOOST_AUTO_TEST_CASE(filter_test)
{
    const http::filter   f1("Keep-Alive, connection,foobar,\r\n goo");

    BOOST_CHECK(f1(_("keep-Alive")));
    BOOST_CHECK(f1(_("Keep-Alive")));
    BOOST_CHECK(!f1(_(" keep-Alive")));

    BOOST_CHECK(f1(_("connection")));
    BOOST_CHECK(f1(_("ConnectIon")));
    BOOST_CHECK(!f1(_("connection ")));

    BOOST_CHECK(f1(_("FooBar")));
    BOOST_CHECK(f1(_("foobar")));
    BOOST_CHECK(!f1(_("foo bar")));

    BOOST_CHECK(!f1(_("keep alive")));
    BOOST_CHECK(!f1(_("keep alive")));

    BOOST_CHECK(f1(_("goo")));
}

static http::filter f(http::filter g)
{
    return g;
}

/**
 * @test tests that two filters can be added
 */
BOOST_AUTO_TEST_CASE(add_filters_test)
{
    http::filter f3, f4;

    {
        const http::filter  f1("a, b, df");
        const http::filter  f2("df,g,h");

        BOOST_CHECK(f1(_("a")));
        BOOST_CHECK(f1(_("b")));
        BOOST_CHECK(f1(_("df")));
        BOOST_CHECK(!f1(_("g")));
        BOOST_CHECK(f2(_("h")));
        BOOST_CHECK(f2(_("g")));
        BOOST_CHECK(f2(_("df")));
        BOOST_CHECK(!f2(_("a")));

        f3 = f1;
        f3 += f2;

        f4 = f2;
    }

    BOOST_CHECK(f3(_("a")));
    BOOST_CHECK(f3(_("b")));
    BOOST_CHECK(f3(_("h")));
    BOOST_CHECK(f3(_("g")));
    BOOST_CHECK(f3(_("df")));

    BOOST_CHECK(f4(_("h")));
    BOOST_CHECK(f4(_("g")));
    BOOST_CHECK(f4(_("df")));

    const http::filter f5(f(f4));
    BOOST_CHECK(f5(_("h")));
    BOOST_CHECK(f5(_("g")));
    BOOST_CHECK(f5(_("df")));

    const http::filter f6("a");
    const http::filter f7("b");
    const http::filter f8 = f6 + f7;

    BOOST_CHECK(f8(_("a")));
    BOOST_CHECK(f8(_("b")));
}
