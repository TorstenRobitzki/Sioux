// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include <boost/test/unit_test.hpp>
#include "tools/substring.h"
#include "tools/iterators.h"

BOOST_AUTO_TEST_CASE(substring_char_pointer_test)
{
    const char buffer1[] = "es war einmal ein baer der schwamm so weit im meer.";
    const char buffer2[] = "es war einmal ein baer der schwamm so weit im meer.";

    // empty strings
    BOOST_CHECK(tools::substring() == tools::substring(tools::begin(buffer1), tools::begin(buffer1)));
    BOOST_CHECK(!(tools::substring() != tools::substring(tools::begin(buffer1), tools::begin(buffer1))));
    BOOST_CHECK(tools::substring(tools::begin(buffer1), tools::begin(buffer1)) == tools::substring());

    const tools::substring s1(tools::begin(buffer1), tools::end(buffer1)-1);
    const tools::substring s2(tools::begin(buffer2), tools::end(buffer2)-1);

    BOOST_CHECK(s1 == s2);
    BOOST_CHECK(s2 == s1);
    BOOST_CHECK(tools::substring(s1) == s1);

    tools::substring s3;
    s3 = s1;
    BOOST_CHECK(s3 == s1 && s3 == s2);

    BOOST_CHECK(!s3.empty());
    BOOST_CHECK(tools::substring().empty());

    BOOST_CHECK(tools::substring(tools::begin(buffer1), tools::begin(buffer1)+5) == "es wa");

    BOOST_CHECK_EQUAL(0u, tools::substring().size());
    BOOST_CHECK_EQUAL(sizeof buffer1 -1, s1.size());
}

BOOST_AUTO_TEST_CASE(trim_test)
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

    BOOST_CHECK_EQUAL(" ababa", s1.trim_right(' '));
    BOOST_CHECK_EQUAL(" ababa", s1.trim_right(' '));
    BOOST_CHECK_EQUAL(" ababa", s1.trim_left('a'));
    BOOST_CHECK_EQUAL("ababa", s1.trim_left(' '));

    BOOST_CHECK_EQUAL("--ab -+- aba++", s2.trim_right('-').trim_left('+'));
    BOOST_CHECK_EQUAL("ab -+- aba", s2.trim_left('-').trim_right('+'));
    BOOST_CHECK_EQUAL("b -+- ab", s2.trim('a'));

    BOOST_CHECK_EQUAL(s3, s3.trim(' '));
    BOOST_CHECK_EQUAL(s3, "aaaaaaaaa");
    BOOST_CHECK_EQUAL("", s3.trim('a'));

    BOOST_CHECK_EQUAL("", s4.trim('a'));

    BOOST_CHECK_EQUAL("ababa", s5.trim('b'));
    BOOST_CHECK_EQUAL("bab", s5.trim('a'));
}

BOOST_AUTO_TEST_CASE( less_compare_test )
{
    const char text1[] = "aa";
    const char text2[] = "a";
    const char text3[] = "zefg";
    const char text4[] = "zefga";
    const char text5[] = "yx";

    tools::substring    s1(tools::begin(text1), tools::end(text1)-1);
    tools::substring    s2(tools::begin(text2), tools::end(text2)-1);
    tools::substring    s3(tools::begin(text3), tools::end(text3)-1);
    tools::substring    s4(tools::begin(text4), tools::end(text4)-1);
    tools::substring    s5(tools::begin(text5), tools::end(text5)-1);

    BOOST_CHECK_EQUAL( s1 < s1, false );
    BOOST_CHECK_EQUAL( s1 < s2, false );
    BOOST_CHECK_EQUAL( s1 < s3, true );

    BOOST_CHECK_EQUAL( s2 < s1, true );
    BOOST_CHECK_EQUAL( s2 < s2, false );
    BOOST_CHECK_EQUAL( s2 < s3, true );

    BOOST_CHECK_EQUAL( s3 < s1, false );
    BOOST_CHECK_EQUAL( s3 < s4, true );
    BOOST_CHECK_EQUAL( s3 < s5, true );

    // some tests with char*
    BOOST_CHECK_EQUAL( s1 < "kjasd", true );
    BOOST_CHECK_EQUAL( s1 < "", false );
    BOOST_CHECK_EQUAL( s1 < "a", false );
    BOOST_CHECK_EQUAL( s4 < "aaaaaaa", true );
}
