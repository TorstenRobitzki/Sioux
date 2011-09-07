// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_TOOLS_IO_SERVICE_H
#define SIOUX_SOURCE_TOOLS_IO_SERVICE_H

#include <boost/asio/io_service.hpp>

namespace tools {

	/**
	 * @brief runs the given io_service until it's queue is empty
	 * @return returns the number of handlers executed
	 */
	std::size_t run( boost::asio::io_service& s );

	/**
	 * @brief runs the given io_service until it's queue is empty
	 * @return returns the number of handlers executed
	 *
	 * The queue is run by a pool of threads in parallel.
	 */
	std::size_t run( boost::asio::io_service& s, unsigned pool_size );

}

#endif // include guard


