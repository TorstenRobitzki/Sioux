// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SERVER_TEST_REQUEST_TEXTS_H
#define SIOUX_SERVER_TEST_REQUEST_TEXTS_H

namespace server {
namespace test {
	const char simple_get_11[] = 
		"GET / HTTP/1.1\r\n"
		"Host: google.de\r\n"
		"User-Agent: Web-sniffer/1.0.31 (+http://web-sniffer.net/)\r\n"
		"Accept-Encoding: gzip\r\n"
		"Accept-Charset: ISO-8859-1,UTF-8;q=0.7,*;q=0.7\r\n"
		"Cache-Control: no\r\n"
		"Accept-Language: de,en;q=0.7,en-us;q=0.3\r\n"
		"Referer: http://web-sniffer.net/\r\n"
		"\r\n";

    const char request_without_end_line[] = 
		"GET / HTTP/1.1\r\n"
		"Host: google.de\r\n"
		"User-Agent: Web-sniffer/1.0.31 (+http://web-sniffer.net/)\r\n"
		"Accept-Encoding: gzip\r\n"
		"Accept-Charset: ISO-8859-1,UTF-8;q=0.7,*;q=0.7\r\n"
		"Cache-Control: no\r\n"
		"Accept-Language: de,en;q=0.7,en-us;q=0.3\r\n"
		"Referer: http://web-sniffer.net/\r\n";

	const char simple_get_11_with_close_header[] = 
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

    template <unsigned S>
    const char* begin(const char(&t)[S])
    {
        return &t[0];
    }

    template <unsigned S>
    const char* end(const char(&t)[S])
    {
        return &t[S-1];
    }

} // namespace test
} // namespace server 

#endif

