#include <boost/system/error_code.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace asio_mocks
{
    /**
     * @brief provide a call back for io functions and record the parameters and the time of the call.
     *
     * to provide reference semantic, each object points to an othere copy of an original created with
     * the default c'tor.
     */
    struct io_completed
    {
        io_completed();
        io_completed(const io_completed& org);
        ~io_completed();
        io_completed& operator=(const io_completed& rhs);

        void swap(io_completed& other);

        void operator()(const boost::system::error_code& e, std::size_t b);

        boost::system::error_code   error;
        size_t                      bytes_transferred;
        boost::posix_time::ptime    when;
        bool                        called;

    private:
        mutable io_completed*       next_;
    };
} // namespace asio_mocks
