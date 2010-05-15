// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/UnitTest++.h"
#include "tools/split.h"
#include <string>

TEST(split_test)
{
    const std::string test1("part1 part2 part3");
    std::string out1l, out1r;
    CHECK(tools::split(test1.begin(), test1.end(), ' ', out1l, out1r));
    CHECK_EQUAL("part1", out1l);
    CHECK_EQUAL("part2 part3", out1r);

    const std::string test2(":part1:part2:");
    std::string out2l, out2r;
    CHECK(!tools::split(test2.begin(), test2.end(), ':', out2l, out2r));
    CHECK(out2l.empty());
    CHECK(out2r.empty());

    const std::string test3("part1    part2 ");
    std::string out3l, out3r;
    CHECK(tools::split(test3.begin(), test3.end(), ' ', out3l, out3r));
    CHECK_EQUAL("part1", out3l);
    CHECK_EQUAL("part2 ", out3r);

    const std::string test4("");
    std::string out4l, out4r;
    CHECK(!tools::split(test4.begin(), test4.end(), 'x', out4l, out4r));
    CHECK(out4l.empty());
    CHECK(out4r.empty());

    const std::string test5("ppp");
    std::string out5l, out5r;
    CHECK(!tools::split(test5.begin(), test5.end(), 'p', out5l, out5r));
    CHECK(out5l.empty());
    CHECK(out5r.empty());
}