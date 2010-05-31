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

    const char get_local_root_opera[] = 
        "GET / HTTP/1.1\r\n"
        "User-Agent: Opera/9.80 (Windows NT 6.1; U; de) Presto/2.5.24 Version/10.53\r\n"
        "Host: 127.0.0.1\r\n"
        "Accept: text/html, application/xml;q=0.9, application/xhtml+xml, image/png, image/jpeg, image/gif, image/x-xbitmap, */*;q=0.1\r\n"
        "Accept-Language: de-DE,de;q=0.9,en;q=0.8\r\n"
        "Accept-Charset: iso-8859-1, utf-8, utf-16, *;q=0.1\r\n"
        "Accept-Encoding: deflate, gzip, x-gzip, identity, *;q=0\r\n"
        "Connection: Keep-Alive\r\n"
        "\r\n";

    const char get_local_root_firefox[] =
        "GET / HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "User-Agent: Mozilla/5.0 (Windows; U; Windows NT 6.1; de; rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3 (.NET CLR 3.5.30729)\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
        "Accept-Language: de-de,de;q=0.8,en-us;q=0.5,en;q=0.3\r\n"
        "Accept-Encoding: gzip,deflate\r\n"
        "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"
        "Keep-Alive: 115\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";

    const char get_local_root_internet_explorer[] =
        "GET / HTTP/1.1\r\n"
        "Accept: application/x-ms-application, image/jpeg, application/xaml+xml, image/gif, image/pjpeg, application/x-ms-xbap, */*\r\n"
        "Accept-Language: de-DE\r\n"
        "User-Agent: Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.1; WOW64; Trident/4.0; SLCC2; .NET CLR 2.0.50727; .NET CLR 3.5.30729; .NET CLR 3.0.30729; Media Center PC 6.0)\r\n"
        "Accept-Encoding: gzip, deflate\r\n"
        "Host: 127.0.0.1\r\n"
        "Connection: Keep-Alive\r\n"
        "\r\n";

    const char cached_response_apache[] = 
        "HTTP/1.1 304 Not Modified\r\n"
        "Date: Mon, 31 May 2010 18:29:26 GMT\r\n"
        "Server: Apache/2.2.15 (Unix)\r\n"
        "Connection: Keep-Alive\r\n"
        "Keep-Alive: timeout=3, max=100\r\n"
        "Etag: \"3101d1a-679-458b06f8562aa\"\r\n"
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

