// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_TEST_TIMER_H
#define SIOUX_SOURCE_SERVER_TEST_TIMER_H

#include <boost/noncopyable.hpp>
#include <boost/asio/io_service.hpp>

namespace server {
    namespace test {
	/**
	 * @brief test replacement of a boost::asio::deadline_timer
	 */
	class timer : boost::noncopyable
	{
	public:
		explicit test_timer(boost::asio::io_service&);
	};
    } // namespace test
} // namespace server

#endif 


