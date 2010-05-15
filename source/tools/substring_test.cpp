// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/unittest++.h"
#include "tools/substring.h"
#include "tools/iterators.h"

TEST(substring_char_pointer_test)
{
    const char buffer1[] = "es war einmal ein baer der schwamm so weit im meer.";
    const char buffer2[] = "es war einmal ein baer der schwamm so weit im meer.";

    // empty strings
    CHECK(tools::substring() == tools::substring(tools::begin(buffer1), tools::begin(buffer1)));
    CHECK(!(tools::substring() != tools::substring(tools::begin(buffer1), tools::begin(buffer1))));
    CHECK(tools::substring(tools::begin(buffer1), tools::begin(buffer1)) == tools::substring());

    const tools::substring s1(tools::begin(buffer1), tools::end(buffer1)-1);
    const tools::substring s2(tools::begin(buffer2), tools::end(buffer2)-1);

    CHECK(s1 == s2);
    CHECK(s2 == s1);
    CHECK(tools::substring(s1) == s1);

    tools::substring s3;
    s3 = s1;
    CHECK(s3 == s1 && s3 == s2);

    CHECK(!s3.empty());
    CHECK(tools::substring().empty());

    CHECK(tools::substring(tools::begin(buffer1), tools::begin(buffer1)+5) == "es wa");

    CHECK_EQUAL(0, tools::substring().size());
    CHECK_EQUAL(sizeof buffer1 -1, s1.size());
}
