// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_HTTP_TEST_REQUEST_TEXTS_H
#define SIOUX_HTTP_TEST_REQUEST_TEXTS_H

namespace http {
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

    // request parameter was "http://asdasdasd"
    const char simple_post[] =
    	"POST / HTTP/1.1\r\n"
    	"Host: web-sniffer.net\r\n"
    	"Origin: http://web-sniffer.net\r\n"
    	"User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_6_8) AppleWebKit/534.50 (KHTML, like Gecko) Version/5.1 Safari/534.50\r\n"
    	"Content-Type: application/x-www-form-urlencoded\r\n"
    	"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
    	"Referer: http://web-sniffer.net/\r\n"
    	"Accept-Language: de-de\r\n"
    	"Accept-Encoding: gzip, deflate\r\n"
    	"Content-Length: 73\r\n"
    	"Connection: keep-alive\r\n"
    	"\r\n"
    	"url=http%3A%2F%2Fasdasdasd&submit=Submit&http=1.1&gzip=yes&type=GET&uak=0";

    const char cached_response_apache[] = 
        "HTTP/1.1 304 Not Modified\r\n"
        "Date: Mon, 31 May 2010 18:29:26 GMT\r\n"
        "Server: Apache/2.2.15 (Unix)\r\n"
        "Connection: Keep-Alive\r\n"
        "Keep-Alive: timeout=3, max=100\r\n"
        "Etag: \"3101d1a-679-458b06f8562aa\"\r\n"
        "\r\n";

    const char ok_response_header_apache[] = 
        "HTTP/1.1 200 OK\r\n"
        "Date: Sun, 11 Jul 2010 08:48:13 GMT\r\n"
        "Server: Apache/2.2.15 (Unix)\r\n"
        "Last-Modified: Tue, 07 Oct 2008 21:25:14 GMT\r\n"
        "Etag: \"3101d29-f99-458b06f7a4ec7\"\r\n"
        "Accept-Ranges: bytes\r\n"
        "Content-Length: 3993\r\n"
        "Keep-Alive: timeout=3, max=99\r\n"
        "Connection: Keep-Alive\r\n"
        "Content-Type: text/css\r\n"
        "\r\n";

    const char chunked_response_example[] = 
        "HTTP/1.1 200 OK\r\n"
        "Date: Mon, 22 Mar 2004 11:15:03 GMT\r\n"
        "Content-Type: text/html\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Trailer: Expires\r\n"
        "\r\n"
        "29\r\n"
        "<html><body><p>The file you requested is \r\n"
        "5;foobar\r\n"
        "3,400\r\n"
        "22\r\n"
        "bytes long and was last modified: \r\n"
        "1d\r\n"
        "Sat, 20 Mar 2004 21:12:00 GMT\r\n"
        "13\r\n"
        ".</p></body></html>\r\n"
        "0\r\n"
        "Expires: Sat, 27 Mar 2004 21:12:00 GMT\r\n"
        "\r\n";

    const char chunked_request_example[] =
        "POST / HTTP/1.1\r\n"
        "Host: web-sniffer.net\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
        "Accept-Language: de-de\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Accept-Encoding: gzip, deflate\r\n"
        "Connection: keep-alive\r\n"
        "\r\n"
        "29\r\n"
        "<html><body><p>The file you requested is \r\n"
        "5;foobar\r\n"
        "3,400\r\n"
        "22\r\n"
        "bytes long and was last modified: \r\n"
        "1d\r\n"
        "Sat, 20 Mar 2004 21:12:00 GMT\r\n"
        "13\r\n"
        ".</p></body></html>\r\n"
        "0\r\n"
        "Expires: Sat, 27 Mar 2004 21:12:00 GMT\r\n"
        "\r\n";

    const char chunked_response_example_body[] =
		"<html><body><p>The file you requested is "
		"3,400"
		"bytes long and was last modified: "
		"Sat, 20 Mar 2004 21:12:00 GMT"
		".</p></body></html>";

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
} // namespace http 

#endif

