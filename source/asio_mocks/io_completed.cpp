#include "asio_mocks/io_completed.h"

namespace asio_mocks
{
    io_completed::io_completed()
        : error()
        , bytes_transferred(0)
        , when()
        , next_(this)
    {
    }

    io_completed::io_completed(const io_completed& org)
        : error(org.error)
        , bytes_transferred(org.bytes_transferred)
        , when(org.when)
        , next_(org.next_)
    {
        org.next_ = this;
    }

    io_completed::~io_completed()
    {
        // unlink this from the cyclic list
        if ( next_ != this )
        {
            io_completed* p = next_;
            for ( ; p->next_ != this; p = p->next_ )
                ;

            p->next_ = next_;
        }
    }

    io_completed& io_completed::operator=(const io_completed& rhs)
    {
        io_completed temp(rhs);
        temp.swap(*this);

        return *this;
    }

    void io_completed::swap(io_completed& other)
    {
        std::swap(error, other.error);
        std::swap(bytes_transferred, other.bytes_transferred);
        std::swap(when, other.when);
        std::swap(next_, other.next_);
    }

    void io_completed::operator()(const boost::system::error_code& e, std::size_t b)
    {
        error = e;
        bytes_transferred = b;
        when = boost::posix_time::microsec_clock::universal_time();

        // update all copies
        for ( io_completed* p = next_; p != this; p = p->next_ )
        {
            p->error = error;
            p->bytes_transferred = bytes_transferred;
            p->when = when;
        }
    }

} // namespace asio_mocks

