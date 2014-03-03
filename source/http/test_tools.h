#ifndef SIOUX_HTTP_TEST_TOOLS_H
#define SIOUX_HTTP_TEST_TOOLS_H

#include <vector>
#include <iosfwd>
#include <boost/random/linear_congruential.hpp>

namespace http
{
    namespace test {

        /**
         * @brief returns a sequence of pseudo-random bytes with the given length
         */
        std::vector<char> random_body(boost::minstd_rand& random, std::size_t size);

        /**
         * @brief returns the given body chunked encoded with the chunks being randomly sized
         */
        std::vector<char> random_chunk(boost::minstd_rand& random, const std::vector<char>& original, std::size_t max_chunk_size);

        /**
         * @brief compares two buffer and produce a "helpful" message, if a difference is found
         */
        bool compare_buffers(const std::vector<char>& org, const std::vector<char>& comp, std::ostream& report);
    } // namespace test

} // namespace server

#endif // include guard

