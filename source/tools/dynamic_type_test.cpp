// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>
#include "tools/dynamic_type.h"

BOOST_AUTO_TEST_CASE(dynamic_type)
{
    struct A { virtual ~A() {} };
    struct B : A {};

    const tools::dynamic_type a(typeid(A));
    const tools::dynamic_type b(typeid(B));
    const tools::dynamic_type i(typeid(int));
    const tools::dynamic_type copy(i);
    const tools::dynamic_type ac(a);

    const A& ab = B();
    A an_a;

    BOOST_CHECK(a == ac);
    BOOST_CHECK(!(a < ac));
    BOOST_CHECK(!(ac < a));
    BOOST_CHECK(a.type() != b.type());
    BOOST_CHECK(a == tools::dynamic_type(typeid(A)));
    BOOST_CHECK(a == tools::dynamic_type(typeid(an_a)));
    BOOST_CHECK(a != tools::dynamic_type(typeid(ab)));

    BOOST_CHECK(a != b);
    BOOST_CHECK(a < b || b < a);
    BOOST_CHECK(a != i);
    BOOST_CHECK(a < i || i < a);
    BOOST_CHECK(i != b);
    BOOST_CHECK(i < b || b < i);
    BOOST_CHECK(i.type() != b.type());
}
