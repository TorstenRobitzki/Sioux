#include "tools/elapse_timer.h"
#include <boost/asio/deadline_timer.hpp>


tools::elapse_timer::elapse_timer()
    : start_(boost::posix_time::microsec_clock::universal_time())
{
}

boost::posix_time::time_duration tools::elapse_timer::elapsed() const
{
    return boost::posix_time::microsec_clock::universal_time() - start_;
}
