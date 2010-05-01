// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_TEST_SOCKET_H
#define SIOUX_SOURCE_SERVER_TEST_SOCKET_H

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio/buffers_iterator.hpp>

#ifdef min
#   undef min
#endif

namespace server {
namespace test {

/**
 * @brief test socket that acts like stream for sending and receiving
 */
template <class Iterator>
class socket
{
public:
    socket(Iterator begin, Iterator end, std::size_t bite_size);

    /**
     * socket interface 
     */
	template<
		typename MutableBufferSequence,
		typename ReadHandler>
	void async_read_some(
		const MutableBufferSequence & buffers,
		ReadHandler handler);

    /**
     * test specific. To be called in a loop until process() returns false
     */
    bool process();
private:
    class handler_keeper_base
    {
    public:
        /*
         * Returns the number of consumed bytes from the range (begin, end]
         */
        virtual std::size_t handle(const boost::system::error_code& error, Iterator begin, Iterator end) = 0;

        virtual ~handler_keeper_base() {}
    };

    template <class MutableBufferSequence, class ReadHandler>
    class handler_keeper : public handler_keeper_base
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
            std::size_t size = std::min<std::size_t>(std::distance(begin, end), buffer_size(buffer_));
            Iterator e = begin;
            std::advance(e, size);
            std::copy(begin, e, boost::asio::buffers_begin(buffer_));

            handler_(error, size);

            return size;
        }

        MutableBufferSequence   buffer_;
        ReadHandler             handler_;
    };

    class impl
    {
    public:
        impl(Iterator begin, Iterator end, std::size_t bite_size);

	    template<
		    typename MutableBufferSequence,
		    typename ReadHandler>
	    void async_read_some(
		    const MutableBufferSequence & buffers,
		    ReadHandler handler);

        bool process();
    private:
	    Iterator			                    current_;
	    const Iterator		                    end_;
        const std::size_t                       bite_size_;
        boost::shared_ptr<handler_keeper_base>  handler_;
    };

    boost::shared_ptr<impl>  pimpl_;
};

// implementation
template <class Iterator>
socket<Iterator>::socket(Iterator begin, Iterator end, std::size_t bite_size)
 : pimpl_(new impl(begin, end, bite_size))
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
bool socket<Iterator>::process()
{
    return pimpl_->process();
}

template <class Iterator>
socket<Iterator>::impl::impl(Iterator begin, Iterator end, std::size_t bite_size)
 : current_(begin)
 , end_(end)
 , bite_size_(bite_size)
 , handler_()
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
bool socket<Iterator>::impl::process()
{
    if ( handler_.get() == 0 )
        return false;

    typedef std::iterator_traits<Iterator>::difference_type size_t;
    size_t size = std::min(static_cast<size_t>(bite_size_), std::distance(current_, end_));
    Iterator end = current_;
    std::advance(end, size);

    size = handler_->handle(make_error_code(boost::system::errc::success), current_, end);
    std::advance(current_, size);

    return true;
}


} // namespace test
} // namespace server

#endif