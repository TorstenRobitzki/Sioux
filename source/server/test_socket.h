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
    socket(Iterator begin, Iterator end, std::size_t bite_size, unsigned times = 1);

    /**
     * @param begin begin of the simulated input
     * @param end end of the simulated input
     * @param read_error the error to deliever, after read_error_occurens bytes where read
     * @param read_error_occurens the number of bytes to be read, before a read error will be simulated
     * @param write_error the error to deliever, after write_error_occurens bytes where written
     * @param write_error_occurens the number of bytes to be written, before a write error will be simulated
     */
    socket(Iterator                         begin, 
           Iterator                         end, 
           const boost::system::error_code& read_error, 
           std::size_t                      read_error_occurens,
           const boost::system::error_code& write_error, 
           std::size_t                      write_error_occurens);

    socket(Iterator begin, Iterator end);

    /**
     * @brief empty implementation
     */
    socket();

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
     * @brief test specific. To be called in a loop until process() returns false
     */
    bool process();

    /**
     * @brief the sampled output
     */
    std::string output() const;
private:
    class read_handler_keeper_base
    {
    public:
        /*
         * Returns the number of consumed bytes from the range (begin, end]
         */
        virtual std::size_t handle(const boost::system::error_code& error, Iterator begin, Iterator end) = 0;

        virtual ~read_handler_keeper_base() {}
    };

    template <class MutableBufferSequence, class ReadHandler>
    class handler_keeper : public read_handler_keeper_base
    {
    public:
        handler_keeper(const MutableBufferSequence& buffer, ReadHandler handler)
            : buffer_(buffer)
            , handler_(handler)
        {
        }

    private:
        virtual std::size_t handle(const boost::system::error_code& error, Iterator begin, Iterator end )
        {
            const std::size_t size = std::min<std::size_t>(std::distance(begin, end), buffer_size(buffer_));
            Iterator e = begin;
            std::advance(e, size);
            std::copy(begin, e, boost::asio::buffers_begin(buffer_));

            handler_(error, size);

            return size;
        }

        MutableBufferSequence   buffer_;
        ReadHandler             handler_;
    };

    class store_writes_base
    {
    public:
        virtual void handler() = 0;
        virtual ~store_writes_base() {}
    };

    template <class WriteHandler>
    class store_writes : public store_writes_base
    {
    public:
        explicit store_writes(WriteHandler h, std::size_t bytes_transferred) 
            : handler_(h)
            , bytes_(bytes_transferred)
        {}
    private:
        virtual void handler()
        {
            handler_(make_error_code(boost::system::errc::success), bytes_);
        }

        const WriteHandler  handler_;
        const std::size_t   bytes_;
    };

    class impl
    {
    public:
        impl();

        impl(Iterator begin, Iterator end, std::size_t bite_size, unsigned times);
    
        impl(Iterator                         begin, 
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
    private:
        bool process_output();
        bool process_input();

	    Iterator			                        current_;
	    const Iterator		                        begin_;
	    const Iterator		                        end_;
        const std::size_t                           bite_size_;
        boost::shared_ptr<read_handler_keeper_base> handler_;
        unsigned                                    times_;

        std::vector<char>                           output_;
        std::deque<boost::shared_ptr<store_writes_base> > writes_;

        const bool                                  read_error_enabled_;
        const boost::system::error_code             read_error_;
        const std::size_t                           read_error_occurens_;

        const bool                                  write_error_enabled_;
        const boost::system::error_code             write_error_;
        const std::size_t                           write_error_occurens_;
    };

    boost::shared_ptr<impl>  pimpl_;
};

// implementation
template <class Iterator>
socket<Iterator>::socket(Iterator begin, Iterator end, std::size_t bite_size, unsigned times)
 : pimpl_(new impl(begin, end, bite_size, times))
{
}

template <class Iterator>
socket<Iterator>::socket(Iterator begin, Iterator end)
 : pimpl_(new impl(begin, end, 0, 1))
{
}

template <class Iterator>
socket<Iterator>::socket(Iterator                         begin, 
                         Iterator                         end, 
                         const boost::system::error_code& read_error, 
                         std::size_t                      read_error_occurens,
                         const boost::system::error_code& write_error, 
                         std::size_t                      write_error_occurens)
 : pimpl_(new impl(begin, end, read_error, read_error_occurens, write_error, write_error_occurens))
{
}

template <class Iterator>
socket<Iterator>::socket()
 : pimpl_(new impl())
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
bool socket<Iterator>::process()
{
    return pimpl_->process();
}

template <class Iterator>
std::string socket<Iterator>::output() const
{
    return pimpl_->output();
}

template <class Iterator>
socket<Iterator>::impl::impl()
 : current_(Iterator())
 , begin_(Iterator())
 , end_(Iterator())
 , bite_size_(0)
 , handler_()
 , times_(0)
 , output_()
 , writes_()
 , read_error_enabled_(false)
 , read_error_()
 , read_error_occurens_()
 , write_error_enabled_(false)
 , write_error_()
 , write_error_occurens_()
{
}

template <class Iterator>
socket<Iterator>::impl::impl(Iterator begin, Iterator end, std::size_t bite_size, unsigned times)
 : current_(begin)
 , begin_(begin)
 , end_(end)
 , bite_size_(bite_size)
 , handler_()
 , times_(times)
 , output_()
 , writes_()
 , read_error_enabled_(false)
 , read_error_()
 , read_error_occurens_()
 , write_error_enabled_(false)
 , write_error_()
 , write_error_occurens_()
{
    assert(times);
}

template <class Iterator>
socket<Iterator>::impl::impl(Iterator                         begin, 
                             Iterator                         end, 
                             const boost::system::error_code& read_error, 
                             std::size_t                      read_error_occurens,
                             const boost::system::error_code& write_error, 
                             std::size_t                      write_error_occurens)
 : current_(begin)
 , begin_(begin)
 , end_(end)
 , bite_size_(0)
 , handler_()
 , times_(1)
 , output_()
 , writes_()
 , read_error_enabled_(true)
 , read_error_()
 , read_error_occurens_()
 , write_error_enabled_(true)
 , write_error_()
 , write_error_occurens_()
{
}

template <class Iterator>
template <typename MutableBufferSequence, typename ReadHandler>
void socket<Iterator>::impl::async_read_some(
		const MutableBufferSequence& buffers,
		ReadHandler handler)
{
    handler_.reset(new handler_keeper<MutableBufferSequence, ReadHandler>(buffers, handler));
}

template <class Iterator>
template<typename ConstBufferSequence, typename WriteHandler>
void socket<Iterator>::impl::async_write_some(
        const ConstBufferSequence & buffers,
        WriteHandler handler)
{
    output_.insert(output_.end(), boost::asio::buffers_begin(buffers), boost::asio::buffers_end(buffers));
    writes_.push_back(boost::shared_ptr<store_writes_base>(new store_writes<WriteHandler>(handler, boost::asio::buffer_size(buffers))));
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
bool socket<Iterator>::impl::process_output()
{
    writes_.front()->handler();
    writes_.pop_front();

    return true;
}

template <class Iterator>
bool socket<Iterator>::impl::process_input()
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

    return true;
}

template <class Iterator>
bool socket<Iterator>::impl::process()
{
    if ( handler_.get() == 0 && writes_.empty() )
        return false;

    return !writes_.empty() ? process_output() : process_input();
}

template <class Iterator>
std::string socket<Iterator>::impl::output() 
{
    const std::string result(output_.begin(), output_.end());
    output_.clear();

    return result;
}



} // namespace test
} // namespace server

#endif