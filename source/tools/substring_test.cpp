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

    CHECK_EQUAL(0u, tools::substring().size());
    CHECK_EQUAL(sizeof buffer1 -1, s1.size());
}

TEST(trim_test)
{
    const char text1[] = " ababa   ";
    const char text2[] = "--ab -+- aba++";
    const char text3[] = "aaaaaaaaa";
    const char text4[] = "";
    const char text5[] = "ababa";

    tools::substring    s1(tools::begin(text1), tools::end(text1)-1);
    tools::substring    s2(tools::begin(text2), tools::end(text2)-1);
    tools::substring    s3(tools::begin(text3), tools::end(text3)-1);
    tools::substring    s4(tools::begin(text4), tools::end(text4)-1);
    tools::substring    s5(tools::begin(text5), tools::end(text5)-1);

    CHECK_EQUAL(" ababa", s1.trim_right(' '));
    CHECK_EQUAL(" ababa", s1.trim_right(' '));
    CHECK_EQUAL(" ababa", s1.trim_left('a'));
    CHECK_EQUAL("ababa", s1.trim_left(' '));

    CHECK_EQUAL("--ab -+- aba++", s2.trim_right('-').trim_left('+'));
    CHECK_EQUAL("ab -+- aba", s2.trim_left('-').trim_right('+'));
    CHECK_EQUAL("b -+- ab", s2.trim('a'));

    CHECK_EQUAL(s3, s3.trim(' '));
    CHECK_EQUAL(s3, "aaaaaaaaa");
    CHECK_EQUAL("", s3.trim('a'));

    CHECK_EQUAL("", s4.trim('a'));

    CHECK_EQUAL("ababa", s5.trim('b'));
    CHECK_EQUAL("bab", s5.trim('a'));
}