#include <boost/date_time/posix_time/ptime.hpp>


namespace tools
{
    /**
     * @brief times the time since construction
     */
    class elapse_timer
    {
    public:
        elapse_timer();

        boost::posix_time::time_duration elapsed() const;
    private:
        boost::posix_time::ptime    start_;
    };
}
