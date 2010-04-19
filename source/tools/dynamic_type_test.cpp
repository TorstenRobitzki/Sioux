// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/UnitTest++.h"
#include "tools/dynamic_type.h"

TEST(dynamic_type)
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

    CHECK(a == ac);
    CHECK(!(a < ac));
    CHECK(!(ac < a));
    CHECK(a.type() != b.type());
    CHECK(a == tools::dynamic_type(typeid(A)));
    CHECK(a == tools::dynamic_type(typeid(an_a)));
    CHECK(a != tools::dynamic_type(typeid(ab)));

    CHECK(a != b);
    CHECK(a < b || b < a);
    CHECK(a != i);
    CHECK(a < i || i < a);
    CHECK(i != b);
    CHECK(i < b || b < i);
    CHECK(i.type() != b.type());
}
