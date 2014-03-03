#ifndef SIOUX_SERVER_TEST_TOOLS_H
#define SIOUX_SERVER_TEST_TOOLS_H

#include <boost/date_time/posix_time/ptime.hpp>

namespace server
{
    namespace test {

        /**
         * @brief waits for the given period and returns then
         */
        void wait(const boost::posix_time::time_duration& period);
    } // namespace test

} // namespace server

#endif // include guard

