// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/unittest++.h"
#include "http/header.h"
#include <cstring>

namespace http {

TEST(parse_http_header)
{
    const char* text1 = "asd:dsa";
    header h;
    CHECK(h.parse(text1, text1 + std::strlen(text1)));
    CHECK_EQUAL("asd:dsa", h.all());
    CHECK_EQUAL("asd", h.name());
    CHECK_EQUAL("dsa", h.value());

    // header can not start with white space
    const char* text2 = " asd:das";
    CHECK(!h.parse(text2, text2 + std::strlen(text2)));

    const char* text3 = "\r\nasd:das";
    CHECK(!h.parse(text3, text3 + std::strlen(text3)));

    // but elsewhere, spaces and tabs are allowed
    const char* text4 = "asd \t : \tdsa \r\n";
    CHECK(h.parse(text4, text4 + std::strlen(text4)));
    CHECK_EQUAL("asd \t : \tdsa \r\n", h.all());
    CHECK_EQUAL("asd", h.name());
    CHECK_EQUAL("dsa", h.value());

    // a header can spawn multiple lines
    const char* text5 = 
        "asd \t : \tdsa \r\n"
        " \r\n foo\r\n";
    CHECK(h.parse(text5, text5 + std::strlen(text5)));
    CHECK_EQUAL("asd \t : \tdsa \r\n \r\n foo\r\n", h.all());
    CHECK_EQUAL("asd", h.name());
    CHECK_EQUAL("dsa \r\n \r\n foo", h.value());
}

} // namespace http 


