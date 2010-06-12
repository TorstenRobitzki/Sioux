// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/unittest++.h"
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
TEST(filter_test)
{
    const http::filter   f1("Keep-Alive, connection,foobar,\r\n goo");

    CHECK(f1(_("keep-Alive")));
    CHECK(f1(_("Keep-Alive")));
    CHECK(!f1(_(" keep-Alive")));

    CHECK(f1(_("connection")));
    CHECK(f1(_("ConnectIon")));
    CHECK(!f1(_("connection ")));

    CHECK(f1(_("FooBar")));
    CHECK(f1(_("foobar")));
    CHECK(!f1(_("foo bar")));

    CHECK(!f1(_("keep alive")));
    CHECK(!f1(_("keep alive")));

    CHECK(f1(_("goo")));
}

static http::filter f(http::filter g)
{
    return g;
}

/**
 * @test tests that two filters can be added
 */
TEST(add_filters_test)
{
    http::filter f3, f4;

    {
        const http::filter  f1("a, b, df");
        const http::filter  f2("df,g,h");

        CHECK(f1(_("a")));
        CHECK(f1(_("b")));
        CHECK(f1(_("df")));
        CHECK(!f1(_("g")));
        CHECK(f2(_("h")));
        CHECK(f2(_("g")));
        CHECK(f2(_("df")));
        CHECK(!f2(_("a")));

        f3 = f1;
        f3 += f2;

        f4 = f2;
    }

    CHECK(f3(_("a")));
    CHECK(f3(_("b")));
    CHECK(f3(_("h")));
    CHECK(f3(_("g")));
    CHECK(f3(_("df")));

    CHECK(f4(_("h")));
    CHECK(f4(_("g")));
    CHECK(f4(_("df")));

    const http::filter f5(f(f4));
    CHECK(f5(_("h")));
    CHECK(f5(_("g")));
    CHECK(f5(_("df")));

    const http::filter f6("a");
    const http::filter f7("b");
    const http::filter f8 = f6 + f7;

    CHECK(f8(_("a")));
    CHECK(f8(_("b")));
}