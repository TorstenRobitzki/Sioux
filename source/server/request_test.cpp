// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/unittest++.h"
#include "server/request.h"
#include "server/test_socket.h"
#include "server/test_timer.h"
#include "server/guide.h"
#include "tools/iterators.h"
#include <boost/thread/barrier.hpp>

namespace {
	template <class Socket, class Guide>
	class test_queue : public server::request_queue<Socket, Guide>
	{
	public:
		test_queue() : barrier_(1) {}

		void wait_for_queued_request()
		{
			barrier_.wait();
		}
	private:
		virtual void request_header_parsed(const server::request<Socket, Guide>&)
		{
		}

		boost::barrier barrier_;

	};

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

TEST(timeout_while_reading_header_test)
{
	typedef server::test_socket<const char*>	socket_t;
	typedef server::guide<server::test_timer>	guide_t;
	typedef server::request<socket_t, guide_t>	request_t;
	typedef test_queue<socket_t, guide_t>		queue_t;
	
	socket_t	socket(tools::begin(request_text), tools::end(request_text));
	guide_t		guide;
	queue_t		queue;
	request_t	request(socket, queue, guide);
	
	queue.wait_for_queued_request();
}

TEST(read_simple_header)
{

	typedef server::test_socket<const char*>	socket_t;
	typedef server::guide<server::test_timer>	guide_t;
	typedef server::request<socket_t, guide_t>	request_t;
	typedef test_queue<socket_t, guide_t>		queue_t;
	
	socket_t	socket(tools::begin(request_text), tools::end(request_text));
	guide_t		guide;
	queue_t		queue;
	request_t	request(socket, queue, guide);
	
	queue.wait_for_queued_request();
}