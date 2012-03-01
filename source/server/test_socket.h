// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_TEST_SOCKET_H
#define SIOUX_SOURCE_SERVER_TEST_SOCKET_H

#include "server/test_io_plan.h"
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/bind/protect.hpp>
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

enum error_on_connect_t { 
    connect_successfully,
    error_on_connect,
    do_not_respond
};

template <error_on_connect_t Result = connect_successfully>
struct connect_error
{
    static error_on_connect_t connect_mode()
    {
        return Result;
    }
};

template <class ConnectError = connect_error<> >
struct socket_behaviour
{
    typedef ConnectError connect_error_t;
};


/**
 * @brief base class for socket, that do not depend on any template parameters
 */
class socket_base
{
public:
	/**
     * @brief simulates that the socket is connected to 192.168.210.1:9999 on the remote side
     */
    boost::asio::ip::tcp::endpoint remote_endpoint() const;

protected:
    ~socket_base() {}
};

/**
 * @brief test socket that acts like stream for sending and receiving
 */
template <class Iterator = const char*, class Trait = socket_behaviour<> >
class socket : public socket_base
{ 
public:
    /**
     * @param io_service the io queue
     * @param begin begin of the simulated input
     * @param end end of the simulated input
     * @param bite_size the size of the simulated network reads. 0 == maximum 
     * @param times number of times, the string (begin, end]
     */
    socket(boost::asio::io_service& io_service, Iterator begin, Iterator end, std::size_t bite_size, unsigned times = 1);

    /**
     * @brief constructs a socket, that issues a delay after every write and read.
     *
     * 
     * @param io_service the io queue
     * @param begin begin of the simulated input
     * @param end end of the simulated input
     * @param bite_size the size of the simulated network reads. 0 == maximum 
     * @param delay delay between two delivered reads
     */
    socket(boost::asio::io_service& io_service, Iterator begin, Iterator end, std::size_t bite_size,
    		const boost::posix_time::time_duration& delay);

    /**
     * @brief constructs a socket, that issues a delay after every write and read.
     *
     * 
     * @param io_service the io queue
     * @param begin begin of the simulated input
     * @param end end of the simulated input
     * @param bite_size the size of the simulated network reads. 0 == maximum 
     * @param read_delay the delay that is issued before asynchron reads are executed
     * @param write_delay the delay that is issued before asynchron writes are executed
     */
    socket(
        boost::asio::io_service& io_service, 
        Iterator begin, 
        Iterator end, 
        std::size_t bite_size, 
        const boost::posix_time::time_duration& read_delay, 
        const boost::posix_time::time_duration& write_delay);

    /**
     * @param io_service the io queue
     * @param begin begin of the simulated input
     * @param end end of the simulated input
     * @param read_error the error to deliever, after read_error_occurens bytes where read.
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

    /**
     * @brief delivers the content given by (begin, end] in maximum sizes
     *
     * @param io_service the io queue
     * @param begin begin of the simulated input
     * @param end end of the simulated input
     */
    socket(boost::asio::io_service& io_service, Iterator begin, Iterator end);

    /**
     * @brief delivers and receives data in chunks of random size.
	 *
     * @param io_service the io queue
     * @param begin begin of the simulated input
     * @param end end of the simulated input
     * @param random random number generator to determine the random sized chunks
     * @param lower_bound the minimum size of a delivered chunk. If less than lower_bound is to be
     *        delivered, the delivered size can fall below of lower_bound.
     * @param upper_bound the maximum size of a delivered chunk.
     */
    socket(boost::asio::io_service& io_service, Iterator begin, Iterator end, 
        const boost::minstd_rand& random, std::size_t lower_bound, std::size_t upper_bound);

    /**
     * @brief empty implementation. Configuration parameters are taken out of the Traits parameter
     */
    explicit socket(boost::asio::io_service& io_service);

    /**
     * @brief socket with explicit plans for reading and writing
     */
    socket(boost::asio::io_service& io_service, const read_plan& reads, const write_plan& writes = write_plan());

    /**
     * @brief default c'tor
     * 
     * This results in a socket that silently doesn't responde to any read or write attempt
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

    template <typename ConnectHandler>
    void async_connect(const boost::asio::ip::tcp::endpoint& peer_endpoint, ConnectHandler handler);

    void close(boost::system::error_code& ec);

    void close();

    void shutdown(
          boost::asio::ip::tcp::socket::shutdown_type what,
          boost::system::error_code& ec);

    void shutdown(boost::asio::ip::tcp::socket::shutdown_type what);

    /**
     * @brief the sampled output
     */
    std::string output() const;

    const std::vector<char>& bin_output() const;

    boost::asio::io_service& get_io_service();

    std::pair<bool, boost::asio::ip::tcp::endpoint> connected() const;

    // returns true, if rhs points to the same implementation
    bool operator==(const socket& rhs) const;
    bool operator!=(const socket& rhs) const;

private:

    class impl : public boost::enable_shared_from_this<impl>
    {
    public:
        impl(boost::asio::io_service& io_service);

        impl(boost::asio::io_service&                   io_service, 
             Iterator                                   begin,
             Iterator                                   end,
             std::size_t                                bite_size, 
             unsigned                                   times,
             const boost::posix_time::time_duration&    read_delay = boost::posix_time::time_duration(),
             const boost::posix_time::time_duration&    write_delay = boost::posix_time::time_duration());
    
        impl(boost::asio::io_service&         io_service,
             Iterator                         begin, 
             Iterator                         end, 
             const boost::system::error_code& read_error, 
             std::size_t                      read_error_occurens,
             const boost::system::error_code& write_error, 
             std::size_t                      write_error_occurens);

        impl(boost::asio::io_service& io_service, Iterator begin, Iterator end, 
            const boost::minstd_rand& random, std::size_t lower_bound, std::size_t upper_bound);

        impl(boost::asio::io_service& io_service, error_on_connect_t);

        impl(boost::asio::io_service& io_service, const read_plan& reads, const write_plan& writes);

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

	    template<
		    typename MutableBufferSequence,
		    typename ReadHandler>
	    void undelayed_async_read_some(
		    const MutableBufferSequence & buffers,
		    ReadHandler handler);

        template<
            typename ConstBufferSequence,
            typename WriteHandler>
        void undelayed_async_write_some(
            const ConstBufferSequence & buffers,
            WriteHandler handler);

        template <typename ConnectHandler>
        void async_connect(const boost::asio::ip::tcp::endpoint& peer_endpoint, ConnectHandler handler);

        void close();

        void shutdown(boost::asio::ip::tcp::socket::shutdown_type what);

        const std::vector<char>& output();

        boost::asio::io_service& get_io_service();

        std::pair<bool, boost::asio::ip::tcp::endpoint> connected() const;
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
        bool                                        connected_;
        bool                                        shutdown_read_;
        bool                                        shutdown_write_;
        boost::asio::ip::tcp::endpoint              endpoint_;

        const bool                                  read_error_enabled_;
        const boost::system::error_code             read_error_;
              std::size_t                           read_error_occurens_;

        const bool                                  write_error_enabled_;
        const boost::system::error_code             write_error_;
              std::size_t                           write_error_occurens_;

        const error_on_connect_t                    connect_error_mode_;

        boost::asio::io_service&                    io_service_;
        boost::asio::deadline_timer                 read_timer_;
        boost::asio::deadline_timer                 write_timer_;
        boost::posix_time::time_duration            read_delay_;
        boost::posix_time::time_duration            write_delay_;

        server::test::read_plan                     read_plan_;
        server::test::write_plan                    write_plan_;
    };

    boost::shared_ptr<impl>  pimpl_;
};

// implementation
template <class Iterator, class Trait>
socket<Iterator, Trait>::socket(boost::asio::io_service& io_service, Iterator begin, Iterator end, std::size_t bite_size, unsigned times)
 : pimpl_(new impl(io_service, begin, end, bite_size, times))
{
}

template <class Iterator, class Trait>
socket<Iterator, Trait>::socket(boost::asio::io_service& io_service, Iterator begin, Iterator end)
 : pimpl_(new impl(io_service, begin, end, 0, 1))
{
}


template <class Iterator, class Trait>
socket<Iterator, Trait>::socket(boost::asio::io_service& io_service, Iterator begin, Iterator end, std::size_t bite_size, const boost::posix_time::time_duration& delay)
  : pimpl_(new impl(io_service, begin, end, bite_size, 1, delay, delay))
{
}

template <class Iterator, class Trait>
socket<Iterator, Trait>::socket(
        boost::asio::io_service& io_service, 
        Iterator begin, 
        Iterator end, 
        std::size_t bite_size, 
        const boost::posix_time::time_duration& read_delay, 
        const boost::posix_time::time_duration& write_delay)
  : pimpl_(new impl(io_service, begin, end, bite_size, 1, read_delay, write_delay))
{
}

template <class Iterator, class Trait>
socket<Iterator, Trait>::socket(boost::asio::io_service&         io_service,
                         Iterator                         begin, 
                         Iterator                         end, 
                         const boost::system::error_code& read_error, 
                         std::size_t                      read_error_occurens,
                         const boost::system::error_code& write_error, 
                         std::size_t                      write_error_occurens)
 : pimpl_(new impl(io_service, begin, end, read_error, read_error_occurens, write_error, write_error_occurens))
{
}

template <class Iterator, class Trait>
socket<Iterator, Trait>::socket(boost::asio::io_service& io_service)
 : pimpl_(new impl(io_service, Trait::connect_error_t::connect_mode()))
{
}

template <class Iterator, class Trait>
socket<Iterator, Trait>::socket(boost::asio::io_service& io_service, Iterator begin, Iterator end,
        const boost::minstd_rand& random, std::size_t lower_bound, std::size_t upper_bound)
 : pimpl_(new impl(io_service, begin, end, random, lower_bound, upper_bound))
{
}

template <class Iterator, class Trait>
socket<Iterator, Trait>::socket(boost::asio::io_service& io_service, const read_plan& reads, const write_plan& writes)
 : pimpl_(new impl(io_service, reads, writes))
{
}

template <class Iterator, class Trait>
socket<Iterator, Trait>::socket()
{
}

template <class Iterator, class Trait>
template <typename MutableBufferSequence, typename ReadHandler>
void socket<Iterator, Trait>::async_read_some(
		const MutableBufferSequence& buffers,
		ReadHandler handler)
{
    pimpl_->async_read_some(buffers, handler);
}

template <class Iterator, class Trait>
template<typename ConstBufferSequence, typename WriteHandler>
void socket<Iterator, Trait>::async_write_some(
        const ConstBufferSequence & buffers,
        WriteHandler handler)
{
    pimpl_->async_write_some(buffers, handler);
}

template <class Iterator, class Trait>
template <typename ConnectHandler>
void socket<Iterator, Trait>::async_connect(const boost::asio::ip::tcp::endpoint& peer_endpoint, ConnectHandler handler)
{
    pimpl_->async_connect(peer_endpoint, handler);
}

template <class Iterator, class Trait>
void socket<Iterator, Trait>::close(boost::system::error_code& ec)
{
    close();
    ec = boost::system::error_code();
}

template <class Iterator, class Trait>
void socket<Iterator, Trait>::close()
{
    pimpl_->close();
}

template <class Iterator, class Trait>
void socket<Iterator, Trait>::shutdown(
          boost::asio::ip::tcp::socket::shutdown_type what,
          boost::system::error_code& ec)
{
    shutdown(what);
    ec = boost::system::error_code();
}   

template <class Iterator, class Trait>
void socket<Iterator, Trait>::shutdown(boost::asio::ip::tcp::socket::shutdown_type what)
{
    pimpl_->shutdown(what);
}

template <class Iterator, class Trait>
std::string socket<Iterator, Trait>::output() const
{
    const std::vector<char>& bin = pimpl_->output();
    return std::string(bin.begin(), bin.end());
}

template <class Iterator, class Trait>
const std::vector<char>& socket<Iterator, Trait>::bin_output() const
{
    return pimpl_->output();
}

template <class Iterator, class Trait>
boost::asio::io_service& socket<Iterator, Trait>::get_io_service()
{
    return pimpl_->get_io_service();
}

template <class Iterator, class Trait>
std::pair<bool, boost::asio::ip::tcp::endpoint> socket<Iterator, Trait>::connected() const
{
    return pimpl_->connected();
}

template <class Iterator, class Trait>
bool socket<Iterator, Trait>::operator==(const socket& rhs) const
{
    return pimpl_ == rhs.pimpl_;
}

template <class Iterator, class Trait>
bool socket<Iterator, Trait>::operator!=(const socket& rhs) const
{
    return pimpl_ != rhs.pimpl_;
}

template <class Iterator, class Trait>
socket<Iterator, Trait>::impl::impl(boost::asio::io_service& io_service)
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
 , connected_(true)
 , shutdown_read_(false)
 , shutdown_write_(false)
 , read_error_enabled_(false)
 , read_error_()
 , read_error_occurens_()
 , write_error_enabled_(false)
 , write_error_()
 , write_error_occurens_()
 , connect_error_mode_(connect_successfully)
 , io_service_(io_service)
 , read_timer_(io_service_)
 , write_timer_(io_service_)
{
}

template <class Iterator, class Trait>
socket<Iterator, Trait>::impl::impl(boost::asio::io_service&                io_service, 
                                    Iterator                                begin, 
                                    Iterator                                end, 
                                    std::size_t                             bite_size, 
                                    unsigned                                times,
                                    const boost::posix_time::time_duration& read_delay,
                                    const boost::posix_time::time_duration& write_delay)
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
 , connected_(true)
 , shutdown_read_(false)
 , shutdown_write_(false)
 , read_error_enabled_(false)
 , read_error_()
 , read_error_occurens_()
 , write_error_enabled_(false)
 , write_error_()
 , write_error_occurens_()
 , connect_error_mode_(connect_successfully)
 , io_service_(io_service)
 , read_timer_(io_service_)
 , write_timer_(io_service_)
 , read_delay_(read_delay)
 , write_delay_(write_delay)
{
    assert(times);
}

template <class Iterator, class Trait>
socket<Iterator, Trait>::impl::impl(boost::asio::io_service&         io_service,
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
 , connected_(true)
 , shutdown_read_(false)
 , shutdown_write_(false)
 , read_error_enabled_(true)
 , read_error_(read_error)
 , read_error_occurens_(read_error_occurens)
 , write_error_enabled_(true)
 , write_error_(write_error)
 , write_error_occurens_(write_error_occurens)
 , connect_error_mode_(connect_successfully)
 , io_service_(io_service)
 , read_timer_(io_service_)
 , write_timer_(io_service_)
{
}

template <class Iterator, class Trait>
socket<Iterator, Trait>::impl::impl(boost::asio::io_service& io_service, Iterator begin, Iterator end, 
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
 , connected_(true)
 , shutdown_read_(false)
 , shutdown_write_(false)
 , read_error_enabled_(false)
 , read_error_()
 , read_error_occurens_()
 , write_error_enabled_(false)
 , write_error_()
 , write_error_occurens_()
 , connect_error_mode_(connect_successfully)
 , io_service_(io_service)
 , read_timer_(io_service_)
 , write_timer_(io_service_)
{
}

template <class Iterator, class Trait>
socket<Iterator, Trait>::impl::impl(boost::asio::io_service& io_service, error_on_connect_t mode)
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
 , connected_(false)
 , shutdown_read_(false)
 , shutdown_write_(false)
 , read_error_enabled_(false)
 , read_error_()
 , read_error_occurens_()
 , write_error_enabled_(false)
 , write_error_()
 , write_error_occurens_()
 , connect_error_mode_(mode)
 , io_service_(io_service)
 , read_timer_(io_service_)
 , write_timer_(io_service_)
{
}
        
template <class Iterator, class Trait>
socket<Iterator, Trait>::impl::impl(boost::asio::io_service& io_service, const read_plan& reads, const write_plan& writes)
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
 , connected_(true)
 , shutdown_read_(false)
 , shutdown_write_(false)
 , read_error_enabled_(false)
 , read_error_()
 , read_error_occurens_()
 , write_error_enabled_(false)
 , write_error_()
 , write_error_occurens_()
 , connect_error_mode_(connect_successfully)
 , io_service_(io_service)
 , read_timer_(io_service_)
 , write_timer_(io_service_)
 , read_plan_(reads)
 , write_plan_(writes)
{
}

namespace {

    // workaround for an issue, where an object that was returned from boost::bind() couldn't directly passed to boost::bind()
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

    template <class Handler, class Buffer, class SocketPtr>
    struct delayed_read_t
    {
        void operator()(const boost::system::error_code& error)
        {
            if ( error )
            {
                handler(make_error_code(boost::asio::error::operation_aborted), 0);
            }
            else
            {
                socket->undelayed_async_read_some(buffer, handler);
            }
        }

        Handler     handler;
        Buffer      buffer;
        SocketPtr   socket;
    };

    template <class Buffer>
    std::size_t copy_read(const std::string& data, Buffer& target)
    {
        const std::size_t size = std::min(data.size(), boost::asio::buffer_size(target));
        std::copy(data.begin(), data.begin()+size, boost::asio::buffers_begin(target));

        return size;
    }

    template <class Handler, class Buffer, class SocketPtr>
    struct delayed_planned_read_t
    {
        void operator()(const boost::system::error_code& error)
        {
            if ( error )
            {
                handler(make_error_code(boost::asio::error::operation_aborted), 0);
            }
            else
            {   
                handler(error, copy_read(data, buffer));
            }
        }

        Handler     handler;
        Buffer      buffer;
        std::string data;
        SocketPtr   socket;
    };


    template <class Handler, class Buffer, class SocketPtr>
    struct delayed_write_t
    {
        void operator()(const boost::system::error_code& error)
        {
            if ( error )
            {
                handler(make_error_code(boost::asio::error::operation_aborted), 0);
            }
            else
            {
                socket->undelayed_async_write_some(buffer, handler);
            }
        }

        Handler     handler;
        Buffer      buffer;
        SocketPtr   socket;
    };

    template <class Handler, class Buffer, class SocketPtr>
    struct delayed_planned_write_t
    {
        void operator()(const boost::system::error_code& error)
        {
            if ( error )
            {
                handler(make_error_code(boost::asio::error::operation_aborted), 0);
            }
            else
            {
                handler(error, size);
            }
        }

        Handler     handler;
        Buffer      buffer;
        std::size_t size;
        SocketPtr   socket;
    };

    template <class Handler, class Buffer, class SocketPtr>
    delayed_read_t<Handler, Buffer, SocketPtr>
    delayed_read(const Handler& handler, const Buffer& buffer, const SocketPtr& socket)
    {
        delayed_read_t<Handler, Buffer, SocketPtr> delay = {handler, buffer, socket};

        return delay;
    }

    template <class Handler, class Buffer, class SocketPtr>
    delayed_planned_read_t<Handler, Buffer, SocketPtr>
    delayed_planned_read(const Handler& handler, const Buffer& buffer, const SocketPtr& socket, const std::string& data)
    {
        delayed_planned_read_t<Handler, Buffer, SocketPtr> delay = {handler, buffer, data, socket};

        return delay;
    }

    template <class Handler, class Buffer, class SocketPtr>
    delayed_write_t<Handler, Buffer, SocketPtr>
    delayed_write(const Handler& handler, const Buffer& buffer, const SocketPtr& socket)
    {
        delayed_write_t<Handler, Buffer, SocketPtr> delay = {handler, buffer, socket};

        return delay;
    }

    template <class Handler, class Buffer, class SocketPtr>
    delayed_planned_write_t<Handler, Buffer, SocketPtr>
    delayed_planned_write(const Handler& handler, const Buffer& buffer, const SocketPtr& socket, std::size_t size)
    {
        delayed_planned_write_t<Handler, Buffer, SocketPtr> delay = {handler, buffer, size, socket};

        return delay;
    }
}

template <class Iterator, class Trait>
template<typename MutableBufferSequence, typename ReadHandler>
void socket<Iterator, Trait>::impl::undelayed_async_read_some(
        const MutableBufferSequence & buffers,
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

    const boost::system::error_code ec = read_error_enabled_ && read_error_occurens_ == 0
        ? read_error_
        : make_error_code(boost::system::errc::success);

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
        io_service_.post(boost::bind<void>(handler, ec, size));
    }
}

template <class Iterator, class Trait>
template <typename MutableBufferSequence, class ReadHandler>
void socket<Iterator, Trait>::impl::async_read_some(
		const MutableBufferSequence& buffers,
		ReadHandler handler)
{
    if ( !connected_ || shutdown_read_ )
    {
        io_service_.post(boost::bind<void>(handler, make_error_code(boost::asio::error::not_connected), 0));
        return;
    }

    if ( !read_plan_.empty() )
    {
        const read_plan::item plan = read_plan_.next_read();

        if ( plan.second != boost::posix_time::time_duration() )
        {
        	read_timer_.expires_from_now(plan.second);
            read_timer_.async_wait(
                delayed_planned_read(handler, buffers, this->shared_from_this(), plan.first));
        }
        else
        {
        	const std::size_t size = copy_read(plan.first, buffers);
            io_service_.post(boost::bind<void>(handler, boost::system::error_code(), size));
        }
    }
    else
    if ( read_delay_ != boost::posix_time::time_duration() )
    {
        read_timer_.expires_from_now(read_delay_);
        read_timer_.async_wait(
            delayed_read(handler, buffers, this->shared_from_this()));
    }
    else
    {
        undelayed_async_read_some(buffers, handler);
    }
}

template <class Iterator, class Trait>
template<typename ConstBufferSequence, typename WriteHandler>
void socket<Iterator, Trait>::impl::undelayed_async_write_some(
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

    const boost::system::error_code ec = write_error_enabled_ && write_error_occurens_ == 0
        ? write_error_
        : make_error_code(boost::system::errc::success);

    if ( write_error_enabled_ )
    {
        size = std::min(size, write_error_occurens_);
        write_error_occurens_ -= size;
    }

    boost::asio::buffers_iterator<ConstBufferSequence> begin = boost::asio::buffers_begin(buffers);
    // std::advance results in a compiler error; this might be a bug in boost::asio
    // vector::insert() complains about boost::asio::buffers_begin(buffers) not implementing random_access_iterator
    for ( std::size_t i = 0; i != size; ++i, ++begin )
    {
    	output_.push_back(*begin);
    }

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

template <class Iterator, class Trait>
template<typename ConstBufferSequence, typename WriteHandler>
void socket<Iterator, Trait>::impl::async_write_some(
        const ConstBufferSequence & buffers,
        WriteHandler handler)
{
    if ( !connected_ || shutdown_write_ )
    {
        io_service_.post(boost::bind<void>(handler, make_error_code(boost::asio::error::not_connected), 0));
        return;
    }

    if ( !write_plan_.empty() )
    {
        const write_plan::item item = write_plan_.next_write();

        if ( item.error_code )
        {
            io_service_.post( boost::bind< void >( handler, item.error_code, 0 ) );
        }
        else if ( item.delay != boost::posix_time::time_duration() )
        {
            write_timer_.expires_from_now(item.delay);
            write_timer_.async_wait(
                delayed_planned_write(handler, buffers, this->shared_from_this(), item.size));
        }
        else
        {
            boost::asio::buffers_iterator<ConstBufferSequence> begin = boost::asio::buffers_begin(buffers);
            boost::asio::buffers_iterator<ConstBufferSequence> end   = boost::asio::buffers_end(buffers);

            std::size_t size = 0;
            // std::advance results in a compiler error; this might be a bug in boost::asio
            for ( std::size_t i = 0; i != item.size && begin != end; ++i, ++begin, ++size )
            {
            	output_.push_back(*begin);
            }

            io_service_.post( boost::bind<void>(handler, make_error_code(boost::system::errc::success), size) );
        }
    }
    else if ( write_delay_ != boost::posix_time::time_duration() )
    {
        write_timer_.expires_from_now(write_delay_);
        write_timer_.async_wait(
            delayed_write(handler, buffers, this->shared_from_this()));
    }
    else
    {
        undelayed_async_write_some(buffers, handler);
    }
}

template <class Iterator, class Trait>
template <typename ConnectHandler>
void socket<Iterator, Trait>::impl::async_connect(const boost::asio::ip::tcp::endpoint& peer_endpoint, ConnectHandler handler)
{
	assert(!connected_);

    if ( connect_error_mode_ == error_on_connect )
    {
        io_service_.post(boost::bind<void>(handler, make_error_code(boost::asio::error::host_not_found)));
    }
    else if ( connect_error_mode_ == connect_successfully )
    {
        io_service_.post(boost::bind<void>(handler, make_error_code(boost::system::errc::success)));
        connected_ = true;
        endpoint_  = peer_endpoint;
    }
}

template <class Iterator, class Trait>
void socket<Iterator, Trait>::impl::close()
{
    connected_ = false;
    read_timer_.cancel();
    write_timer_.cancel();
}

template <class Iterator, class Trait>
void socket<Iterator, Trait>::impl::shutdown(boost::asio::ip::tcp::socket::shutdown_type what)
{
    if ( what == boost::asio::ip::tcp::socket::shutdown_both || what == boost::asio::ip::tcp::socket::shutdown_receive )
    {
        shutdown_read_ = true;
    }

    if ( what == boost::asio::ip::tcp::socket::shutdown_both || what == boost::asio::ip::tcp::socket::shutdown_send )
    {
        shutdown_write_ = true;
    }
}

template <class Iterator, class Trait>
const std::vector<char>& socket<Iterator, Trait>::impl::output() 
{
    return output_;
}

template <class Iterator, class Trait>
boost::asio::io_service& socket<Iterator, Trait>::impl::get_io_service()
{
    return io_service_;
}

template <class Iterator, class Trait>
std::pair<bool, boost::asio::ip::tcp::endpoint> socket<Iterator, Trait>::impl::connected() const
{
    return std::make_pair(connected_, endpoint_);
}

} // namespace test
} // namespace server

#endif

