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
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/function.hpp>
#include <vector>
#include <deque>
#include <iterator>

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
     * @brief deleviers and receives data in chunks of random size.
     */
    socket(boost::asio::io_service& io_service, Iterator begin, Iterator end, 
        const boost::minstd_rand& random, std::size_t lower_bound, std::size_t upper_bound);

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

    const std::vector<char>& bin_output() const;

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

        impl(boost::asio::io_service& io_service, Iterator begin, Iterator end, 
            const boost::minstd_rand& random, std::size_t lower_bound, std::size_t upper_bound);

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

        const std::vector<char>& output();

        boost::asio::io_service& get_io_service();
    private:
        typedef boost::uniform_int<std::size_t>                                     
            rand_distribution_type;
        typedef boost::variate_generator<boost::minstd_rand&, rand_distribution_type>    
            rand_gen_type;

        Iterator			                        current_;
	    const Iterator		                        begin_;
	    const Iterator		                        end_;

        const std::size_t                           bite_size_;
        unsigned                                    times_;
        
        bool                                        use_random_generator_;
        boost::minstd_rand                          random_generator_;
        rand_distribution_type                      random_destribution_;
        rand_gen_type                               random_;
    
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
socket<Iterator>::socket(boost::asio::io_service& io_service, Iterator begin, Iterator end, 
        const boost::minstd_rand& random, std::size_t lower_bound, std::size_t upper_bound)
 : pimpl_(new impl(io_service, begin, end, random, lower_bound, upper_bound))
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
    const std::vector<char>& bin = pimpl_->output();
    return std::string(bin.begin(), bin.end());
}

template <class Iterator>
const std::vector<char>& socket<Iterator>::bin_output() const
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
 , use_random_generator_(false)
 , random_generator_()
 , random_destribution_()
 , random_(random_generator_, random_destribution_)
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
 , use_random_generator_(false)
 , random_generator_()
 , random_destribution_()
 , random_(random_generator_, random_destribution_)
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
 , use_random_generator_(false)
 , random_generator_()
 , random_destribution_()
 , random_(random_generator_, random_destribution_)
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
socket<Iterator>::impl::impl(boost::asio::io_service& io_service, Iterator begin, Iterator end, 
    const boost::minstd_rand& random, std::size_t lower_bound, std::size_t upper_bound)
 : current_(begin)
 , begin_(begin)
 , end_(end)
 , bite_size_(0)
 , times_(1)
 , use_random_generator_(true)
 , random_generator_(random)
 , random_destribution_(lower_bound, upper_bound)
 , random_(random_generator_, random_destribution_)
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

namespace {

    // workaround for an issue, where a object that was returned from boost::bind() couldn't directly passed to boost::bind()
    template <class Handler>
    struct handler_container
    {
        explicit handler_container(const Handler& h) : handler(h) {}

        Handler handler;
    };

    template <typename Container>
    void repost( boost::asio::io_service& io, Container handler, boost::system::error_code ec, std::size_t bytes_transferred)
    {
        io.post(boost::bind<void>(handler.handler, ec, bytes_transferred));
    }

}

template <class Iterator>
template <typename MutableBufferSequence, class ReadHandler>
void socket<Iterator>::impl::async_read_some(
		const MutableBufferSequence& buffers,
		ReadHandler handler)
{
    std::size_t size = std::min<std::size_t>(std::distance(current_, end_), buffer_size(buffers));

    if ( bite_size_ != 0 )
    {
        size = std::min(size, bite_size_);
    }

    bool repost_result = false;
    if ( use_random_generator_ )
    {
        size = std::min(size, random_());
        repost_result = random_() % 2 == 1;
    }

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

    if ( repost_result )
    {
        io_service_.post(
            boost::bind(
                repost<handler_container<ReadHandler> >, 
                boost::ref(io_service_), 
                handler_container<ReadHandler>(handler), 
                ec, 
                size
            )
        );
    }
    else
    {
        io_service_.post(boost::bind(handler, ec, size));
    }
}

template <class Iterator>
template<typename ConstBufferSequence, typename WriteHandler>
void socket<Iterator>::impl::async_write_some(
        const ConstBufferSequence & buffers,
        WriteHandler handler)
{
    std::size_t size = std::distance(boost::asio::buffers_begin(buffers), boost::asio::buffers_end(buffers));

    bool repost_result = false;
    if ( use_random_generator_ )
    {
        size = std::min(size, random_());
        repost_result = random_() % 2 == 1;
    }

    if ( write_error_enabled_ )
    {
        size = std::min(size, write_error_occurens_);
        write_error_occurens_ -= size;
    }

    boost::asio::buffers_iterator<ConstBufferSequence> end = boost::asio::buffers_begin(buffers);
    // std::advance results in a compiler error; this might be a bug in boost::asio
    for ( std::size_t i = 0; i != size; ++i )
        ++end;

    output_.insert(output_.end(), boost::asio::buffers_begin(buffers), end);

    const boost::system::error_code ec = write_error_enabled_ && write_error_occurens_ == 0 
        ? write_error_
        : make_error_code(boost::system::errc::success);

    if ( repost_result )
    {
        io_service_.post(
            boost::bind(
                repost<handler_container<WriteHandler> >, 
                boost::ref(io_service_), 
                handler_container<WriteHandler>(handler), 
                ec, 
                size
            )
        );
    }
    else
    {
        io_service_.post(boost::bind<void>(handler, ec, size));
    }
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
const std::vector<char>& socket<Iterator>::impl::output() 
{
    return output_;
}

template <class Iterator>
boost::asio::io_service& socket<Iterator>::impl::get_io_service()
{
    return io_service_;
}




} // namespace test
} // namespace server

#endif