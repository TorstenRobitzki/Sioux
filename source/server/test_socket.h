// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_TEST_SOCKET_H
#define SIOUX_SOURCE_SERVER_TEST_SOCKET_H

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <vector>
#include <deque>

#ifdef min
#   undef min
#endif

namespace server {

/** 
 * @namespace server::test 
 * namespace for test equipment 
 */
namespace test {

/**
 * @brief test socket that acts like stream for sending and receiving
 */
template <class Iterator>
class socket
{
public:
    /**
     * @param begin begin of the simulated input
     * @param end end of the simulated input
     * @param bite_size the size of the simulated network reads. 0 == maximum 
     * @param times number of times, the string (begin, end]
     */
    socket(boost::asio::io_service& io_service, Iterator begin, Iterator end, std::size_t bite_size, unsigned times = 1);

    /**
     * @param begin begin of the simulated input
     * @param end end of the simulated input
     * @param read_error the error to deliever, after read_error_occurens bytes where read
     * @param read_error_occurens the number of bytes to be read, before a read error will be simulated
     * @param write_error the error to deliever, after write_error_occurens bytes where written
     * @param write_error_occurens the number of bytes to be written, before a write error will be simulated
     */
    socket(boost::asio::io_service&         io_service, 
           Iterator                         begin, 
           Iterator                         end, 
           const boost::system::error_code& read_error, 
           std::size_t                      read_error_occurens,
           const boost::system::error_code& write_error, 
           std::size_t                      write_error_occurens);

    socket(boost::asio::io_service& io_service, Iterator begin, Iterator end);

    /**
     * @brief empty implementation
     */
    socket(boost::asio::io_service& io_service);

    /**
     * socket interface 
     */
	template<
		typename MutableBufferSequence,
		typename ReadHandler>
	void async_read_some(
		const MutableBufferSequence & buffers,
		ReadHandler handler);

    template<
        typename ConstBufferSequence,
        typename WriteHandler>
    void async_write_some(
        const ConstBufferSequence & buffers,
        WriteHandler handler);

    void close();

    void shutdown(boost::asio::ip::tcp::socket::shutdown_type what);

    /**
     * @brief the sampled output
     */
    std::string output() const;

    boost::asio::io_service& get_io_service();
private:

    class impl
    {
    public:
        impl(boost::asio::io_service& io_service);

        impl(boost::asio::io_service& io_service, Iterator begin, Iterator end, std::size_t bite_size, unsigned times);
    
        impl(boost::asio::io_service&         io_service,
             Iterator                         begin, 
             Iterator                         end, 
             const boost::system::error_code& read_error, 
             std::size_t                      read_error_occurens,
             const boost::system::error_code& write_error, 
             std::size_t                      write_error_occurens);

	    template<
		    typename MutableBufferSequence,
		    typename ReadHandler>
	    void async_read_some(
		    const MutableBufferSequence & buffers,
		    ReadHandler handler);

        template<
            typename ConstBufferSequence,
            typename WriteHandler>
        void async_write_some(
            const ConstBufferSequence & buffers,
            WriteHandler handler);

        void close();

        void shutdown(boost::asio::ip::tcp::socket::shutdown_type what);

        bool process();

        std::string output();

        boost::asio::io_service& get_io_service();
    private:
        void process_output();
        void process_input();

	    Iterator			                        current_;
	    const Iterator		                        begin_;
	    const Iterator		                        end_;

        const std::size_t                           bite_size_;
        unsigned                                    times_;

        std::vector<char>                           output_;

        const bool                                  read_error_enabled_;
        const boost::system::error_code             read_error_;
              std::size_t                           read_error_occurens_;

        const bool                                  write_error_enabled_;
        const boost::system::error_code             write_error_;
              std::size_t                           write_error_occurens_;

        boost::asio::io_service&                    io_service_;
    };

    boost::shared_ptr<impl>  pimpl_;
};

// implementation
template <class Iterator>
socket<Iterator>::socket(boost::asio::io_service& io_service, Iterator begin, Iterator end, std::size_t bite_size, unsigned times)
 : pimpl_(new impl(io_service, begin, end, bite_size, times))
{
}

template <class Iterator>
socket<Iterator>::socket(boost::asio::io_service& io_service, Iterator begin, Iterator end)
 : pimpl_(new impl(io_service, begin, end, 0, 1))
{
}

template <class Iterator>
socket<Iterator>::socket(boost::asio::io_service&         io_service,
                         Iterator                         begin, 
                         Iterator                         end, 
                         const boost::system::error_code& read_error, 
                         std::size_t                      read_error_occurens,
                         const boost::system::error_code& write_error, 
                         std::size_t                      write_error_occurens)
 : pimpl_(new impl(io_service, begin, end, read_error, read_error_occurens, write_error, write_error_occurens))
{
}

template <class Iterator>
socket<Iterator>::socket(boost::asio::io_service& io_service)
 : pimpl_(new impl(io_service))
{
}

template <class Iterator>
template <typename MutableBufferSequence, typename ReadHandler>
void socket<Iterator>::async_read_some(
		const MutableBufferSequence& buffers,
		ReadHandler handler)
{
    pimpl_->async_read_some(buffers, handler);
}

template <class Iterator>
template<typename ConstBufferSequence, typename WriteHandler>
void socket<Iterator>::async_write_some(
        const ConstBufferSequence & buffers,
        WriteHandler handler)
{
    pimpl_->async_write_some(buffers, handler);
}

template <class Iterator>
void socket<Iterator>::close()
{
    pimpl_->close();
}

template <class Iterator>
void socket<Iterator>::shutdown(boost::asio::ip::tcp::socket::shutdown_type what)
{
    pimpl_->shutdown(what);
}

template <class Iterator>
std::string socket<Iterator>::output() const
{
    return pimpl_->output();
}

template <class Iterator>
boost::asio::io_service& socket<Iterator>::get_io_service()
{
    return pimpl_->get_io_service();
}


template <class Iterator>
socket<Iterator>::impl::impl(boost::asio::io_service& io_service)
 : current_(Iterator())
 , begin_(Iterator())
 , end_(Iterator())
 , bite_size_(0)
 , times_(0)
 , output_()
 , read_error_enabled_(false)
 , read_error_()
 , read_error_occurens_()
 , write_error_enabled_(false)
 , write_error_()
 , write_error_occurens_()
 , io_service_(io_service)
{
}

template <class Iterator>
socket<Iterator>::impl::impl(boost::asio::io_service& io_service, Iterator begin, Iterator end, std::size_t bite_size, unsigned times)
 : current_(begin)
 , begin_(begin)
 , end_(end)
 , bite_size_(bite_size)
 , times_(times)
 , output_()
 , read_error_enabled_(false)
 , read_error_()
 , read_error_occurens_()
 , write_error_enabled_(false)
 , write_error_()
 , write_error_occurens_()
 , io_service_(io_service)
{
    assert(times);
}

template <class Iterator>
socket<Iterator>::impl::impl(boost::asio::io_service&         io_service,
                             Iterator                         begin, 
                             Iterator                         end, 
                             const boost::system::error_code& read_error, 
                             std::size_t                      read_error_occurens,
                             const boost::system::error_code& write_error, 
                             std::size_t                      write_error_occurens)
 : current_(begin)
 , begin_(begin)
 , end_(end)
 , bite_size_(0)
 , times_(1)
 , output_()
 , read_error_enabled_(true)
 , read_error_(read_error)
 , read_error_occurens_(read_error_occurens)
 , write_error_enabled_(true)
 , write_error_(write_error)
 , write_error_occurens_(write_error_occurens)
 , io_service_(io_service)
{
}

template <class Iterator>
template <typename MutableBufferSequence, typename ReadHandler>
void socket<Iterator>::impl::async_read_some(
		const MutableBufferSequence& buffers,
		ReadHandler handler)
{
    std::size_t size = std::min<std::size_t>(std::distance(current_, end_), buffer_size(buffers));

    if ( bite_size_ != 0 )
        size = std::min(size, bite_size_);

    if ( read_error_enabled_ )
    {
        size = std::min(size, read_error_occurens_);
        read_error_occurens_ -= size;
    }

    Iterator e = current_;
    std::advance(e, size);
    std::copy(current_, e, boost::asio::buffers_begin(buffers));
    current_ = e;

    if ( current_ == end_ && --times_ != 0 )
    {
        current_ = begin_;
    }

    boost::system::error_code ec = read_error_enabled_ && read_error_occurens_ == 0 
        ? make_error_code(boost::system::errc::success)
        : read_error_;

    io_service_.post(boost::bind(handler, ec, size));
}

template <class Iterator>
template<typename ConstBufferSequence, typename WriteHandler>
void socket<Iterator>::impl::async_write_some(
        const ConstBufferSequence & buffers,
        WriteHandler handler)
{
    output_.insert(output_.end(), boost::asio::buffers_begin(buffers), boost::asio::buffers_end(buffers));

    const std::size_t size = std::distance(boost::asio::buffers_begin(buffers), boost::asio::buffers_end(buffers));

    io_service_.post(boost::bind<void>(handler, make_error_code(boost::system::errc::success), size));
}

template <class Iterator>
void socket<Iterator>::impl::close()
{
}

template <class Iterator>
void socket<Iterator>::impl::shutdown(boost::asio::ip::tcp::socket::shutdown_type /*what*/)
{
}

template <class Iterator>
void socket<Iterator>::impl::process_output()
{
    writes_.front()->handler();
    writes_.pop_front();
}

template <class Iterator>
void socket<Iterator>::impl::process_input()
{
    if ( current_ == end_ && --times_ )
        current_ = begin_;

    typedef std::iterator_traits<Iterator>::difference_type size_t;

    size_t size = bite_size_ != 0
        ? std::min(static_cast<size_t>(bite_size_), std::distance(current_, end_))
        : std::distance(current_, end_);

    Iterator end = current_;
    std::advance(end, size);

    boost::shared_ptr<read_handler_keeper_base>  old_handler;
    old_handler.swap(handler_);

    size = old_handler->handle(make_error_code(boost::system::errc::success), current_, end);
    std::advance(current_, size);
}

template <class Iterator>
bool socket<Iterator>::impl::process()
{
    if ( handler_.get() == 0 && writes_.empty() )
        return false;

    writes_.empty() ? process_input() : process_output();

    return true;
}

template <class Iterator>
std::string socket<Iterator>::impl::output() 
{
    const std::string result(output_.begin(), output_.end());
    output_.clear();

    return result;
}

template <class Iterator>
boost::asio::io_service& socket<Iterator>::impl::get_io_service()
{
    return io_service_;
}




} // namespace test
} // namespace server

#endif