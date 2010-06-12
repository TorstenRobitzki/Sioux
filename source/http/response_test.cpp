// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "http/response.h"
#include "unittest++/unittest++.h"
#include "http/test_request_texts.h"

using namespace http;
using namespace http::test;

TEST(broken_response_line)
{
    const response_header with_tabs("HTTP/1.1\t100\tI don't know\r\n\r\n");    
    CHECK_EQUAL(http::message::syntax_error, with_tabs.state());

    const response_header response_code_out_of_lower_range("HTTP/1.1 99 I don't know\r\n\r\n");    
    CHECK_EQUAL(http::message::syntax_error, response_code_out_of_lower_range.state());

    const response_header response_code_out_of_upper_range("HTTP/1.1 600 I don't know\r\n\r\n");    
    CHECK_EQUAL(http::message::syntax_error, response_code_out_of_upper_range.state());

    const response_header broken_version("HTTP\\1.1 100 I don't know\r\n\r\n");    
    CHECK_EQUAL(http::message::syntax_error, broken_version.state());

    const response_header missing_code("HTTP/4.4  I don't know\r\n\r\n");    
    CHECK_EQUAL(http::message::syntax_error, missing_code.state());

    const response_header missing_end_line("HTTP/1.1 100 ok\r\n");
    CHECK_EQUAL(http::message::parsing, missing_end_line.state());
}

TEST(valid_response_line)
{
    const response_header empty_phrase("HTTP/1.1 222\r\n\r\n");
    CHECK_EQUAL(message::ok, empty_phrase.state());
    CHECK_EQUAL(1001u, empty_phrase.milli_version());
    CHECK_EQUAL(static_cast<http_error_code>(222), empty_phrase.code());

    const response_header smallest_code("HTTp/2.2 100 Ok\r\n\r\n");
    CHECK_EQUAL(message::ok, smallest_code.state());
    CHECK_EQUAL(2u, smallest_code.major_version());
    CHECK_EQUAL(2u, smallest_code.minor_version());
    CHECK_EQUAL(http_continue, smallest_code.code());

    const response_header highest_code("http/12.21 599 WTF!\r\n\r\n");
    CHECK_EQUAL(message::ok, highest_code.state());
    CHECK_EQUAL(12u, highest_code.major_version());
    CHECK_EQUAL(21u, highest_code.minor_version());
    CHECK_EQUAL(static_cast<http_error_code>(599), highest_code.code());
    CHECK_EQUAL("WTF!", highest_code.phrase());
}