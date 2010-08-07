// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SERVER_TEST_TOOLS_H
#define SIOUX_SERVER_TEST_TOOLS_H

#include <vector>
#include <iosfwd>
#include <boost/asio/io_service.hpp>
#include <boost/random/linear_congruential.hpp>

namespace server
{
    namespace test {

        /**
         * @brief runs the given io_service until it's queue is empty
         * @return returns the number of handlers executed
         */
        std::size_t run(boost::asio::io_service& s);

        /**
         * @brief runs the given io_service until it's queue is empty
         * @return returns the number of handlers executed
         *
         * The queue is run by a pool of threads in paralel. 
         */
        std::size_t run(boost::asio::io_service& s, unsigned pool_size);

        /**
         * @brief returns a sequence of pseudo-random bytes with the given length
         */
        std::vector<char> random_body(boost::minstd_rand& random, std::size_t size);

        /**
         * @brief returns the given body chunked encoding with the chunks beeing randomly sized
         */
        std::vector<char> random_chunk(boost::minstd_rand& random, const std::vector<char>& original, std::size_t max_chunk_size);

        /**
         * @brief compares two buffer and produce a helpfull message, if a difference is found
         */
        bool compare_buffers(const std::vector<char>& org, const std::vector<char>& comp, std::ostream& report);

    } // namespace test

} // namespace server

#endif // include guard

