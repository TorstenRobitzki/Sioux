// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/unittest++.h"
#include "server/connection.h"
#include "server/test_traits.h"
#include "tools/iterators.h"

// delete me if you see me
#include <iostream>

namespace
{
	const char request_text[] = 
		"GET / HTTP/1.1\r\n"
		"Host: google.de\r\n"
		"Connection: close\r\n"
		"User-Agent: Web-sniffer/1.0.31 (+http://web-sniffer.net/)\r\n"
		"Accept-Encoding: gzip\r\n"
		"Accept-Charset: ISO-8859-1,UTF-8;q=0.7,*;q=0.7\r\n"
		"Cache-Control: no\r\n"
		"Accept-Language: de,en;q=0.7,en-us;q=0.3\r\n"
		"Referer: http://web-sniffer.net/\r\n"
		"\r\n";

}

TEST(read_simple_header)
{
    server::test::traits<>::connection_type	socket(tools::begin(request_text), tools::end(request_text), 5);

    server::create_connection(socket, server::test::traits<>());

    for (; socket.process(); )
        ;

}