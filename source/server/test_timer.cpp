// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/test_timer.h"

#include <cassert>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <map>
#include <vector>

namespace
{
    typedef boost::function< void ( const boost::system::error_code& ) > time_cb_t;
    typedef boost::posix_time::ptime time_t;

    const time_t time_zero = boost::posix_time::time_from_string( "1970-01-01 00:00:00" );

    struct timer_data
    {
        std::vector< time_cb_t >    call_backs_;
        boost::asio::io_service*    queue_;
        time_t                      expiration_time_;

        explicit timer_data( boost::asio::io_service* q )
            : queue_( q )
            , expiration_time_( boost::posix_time::pos_infin )
        {
        }
    };

    class implemenation
    {
    public:
        std::size_t expires_at( const time_t& expiration_time, boost::asio::io_service& queue,
            const server::test::timer& timer )
        {
            boost::mutex::scoped_lock lock( mutex_ );
            timer_data& data = find_timer( timer, queue );

            invoke_handlers( data, make_error_code( boost::asio::error::operation_aborted ) );

            const std::size_t result = data.call_backs_.size();
            data.call_backs_.clear();
            data.expiration_time_ = expiration_time;

            return result;
        }

        void add_expiration_handler( const time_cb_t& handler, boost::asio::io_service& queue,
            const server::test::timer& timer )
        {
            boost::mutex::scoped_lock lock( mutex_ );

            timer_data& data = find_timer( timer, queue );
            data.call_backs_.push_back( handler );
        }

        std::size_t cancel( boost::asio::io_service& queue, const server::test::timer& timer )
        {
            boost::mutex::scoped_lock lock( mutex_ );

            timer_data callbacks = remove_timer( timer, queue );
            const std::size_t result = callbacks.call_backs_.size();

            invoke_handlers( callbacks, boost::asio::error::operation_aborted );

            return result;
        }

        unsigned advance_time()
        {
            boost::mutex::scoped_lock lock( mutex_ );

            if ( timers_.empty() )
                return 0;

            boost::posix_time::ptime min = boost::posix_time::max_date_time;
            for ( timer_map_t::iterator timer = timers_.begin(); timer != timers_.end(); ++timer )
            {
                min = std::min( min, timer->second.expiration_time_ );
            }

            return current_time_impl( min );
        }

        time_t current_time()
        {
            boost::mutex::scoped_lock lock( mutex_ );

            if ( current_time_ == time_t() )
                current_time_ = time_zero;

            return current_time_;
        }

        void current_time( const boost::posix_time::ptime& new_time )
        {
            boost::mutex::scoped_lock lock( mutex_ );
            current_time_impl( new_time );
        }

    private:
        unsigned current_time_impl( const boost::posix_time::ptime& new_time )
        {
            current_time_ = new_time;
            unsigned result = 0;

            for ( timer_map_t::iterator timer = timers_.begin(); timer != timers_.end(); )
            {
                if  ( timer->second.expiration_time_ <= current_time_ )
                {
                    invoke_handlers( timer->second, boost::system::error_code() );
                    timers_.erase( timer++ );
                    ++result;
                }
                else
                {
                    ++timer;
                }
            }

            return result;
        }

        void invoke_handlers( const timer_data& timer, const boost::system::error_code& ec )
        {
            for ( std::vector< time_cb_t >::const_iterator cb = timer.call_backs_.begin();
                cb != timer.call_backs_.end(); ++cb )
            {
                timer.queue_->post( boost::bind( *cb, ec ) );
            }
        }

        timer_data& find_timer( const server::test::timer& timer, boost::asio::io_service& queue )
        {
            timer_map_t::iterator pos = timers_.find( &timer );

            assert( pos == timers_.end() || pos->second.queue_ == &queue );

            if ( pos == timers_.end() )
            {
                pos = timers_.insert( std::make_pair( &timer, timer_data( &queue ) ) ).first;
            }

            return pos->second;
        }

        timer_data remove_timer( const server::test::timer& timer, boost::asio::io_service& queue )
        {
            timer_map_t::iterator pos = timers_.find( &timer );

            const timer_data result = pos != timers_.end() ? pos->second : timer_data( &queue );

            if ( pos != timers_.end() )
            {
                assert( pos->second.queue_ == &queue );
                timers_.erase( pos );
            }

            return result;
        }

        mutable boost::mutex    mutex_;

        typedef std::map< const server::test::timer*, timer_data > timer_map_t;
        timer_map_t             timers_;
        time_t                  current_time_;
    };

    implemenation& impl()
    {
        static implemenation impl;
        return impl;
    }
}

namespace server {
namespace test {


    timer::timer( boost::asio::io_service& q )
        : queue_( q )
    {
    }

    timer::~timer()
    {
        cancel();
    }

    std::size_t timer::cancel()
    {
        return impl().cancel( queue_, *this );
    }

    timer::time_type timer::expires_at() const
    {
        return time_type();
    }

    std::size_t timer::expires_at( const time_type & expiry_time )
    {
        return impl().expires_at( expiry_time, queue_, *this );
    }

    std::size_t timer::expires_at( const time_type & expiry_time, boost::system::error_code & ec )
    {
        ec = boost::system::error_code();
        return expires_at( expiry_time );
    }

    timer::duration_type timer::expires_from_now() const
    {
        return duration_type();
    }

    std::size_t timer::expires_from_now( const duration_type & expiry_time )
    {
        return impl().expires_at( impl().current_time() + expiry_time, queue_, *this );
    }

    std::size_t timer::expires_from_now( const duration_type & expiry_time, boost::system::error_code & ec )
    {
        ec = boost::system::error_code();
        return expires_from_now( expiry_time );
    }

    void timer::async_wait_impl( const boost::function< void ( const boost::system::error_code& ) >& handler )
    {
        impl().add_expiration_handler( handler, queue_, *this );
    }

    boost::posix_time::ptime current_time()
    {
        return impl().current_time();
    }

    void current_time( const boost::posix_time::ptime& new_time )
    {
        impl().current_time( new_time );
    }

    void reset_time()
    {
        impl().current_time( time_zero );
    }

    void advance_time( const boost::posix_time::time_duration& delay )
    {
        current_time( current_time() + delay );
    }

    unsigned advance_time()
    {
        return impl().advance_time();
    }

}
}
