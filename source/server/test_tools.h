// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SERVER_TEST_TOOLS_H
#define SIOUX_SERVER_TEST_TOOLS_H

#include <boost/asio/io_service.hpp>

namespace server
{
    namespace test {

        /**
         * @brief runs the given io_service until it's queue is empty
         * @return returns the number of handlers executed
         */
        std::size_t run(boost::asio::io_service& s);
    } // namespace test

} // namespace server

#endif // include guard

