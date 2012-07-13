// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "rack/ruby_land_queue.h"
#include <boost/thread/reverse_lock.hpp>
#include <ruby.h>

namespace rack
{

    ruby_land_queue::ruby_land_queue()
     : mutex_()
     , condition_()
     , stop_flag_( false )
     , queue_()
    {
    }

    bool ruby_land_queue::push( const call_back_t& request )
    {
        boost::mutex::scoped_lock lock( mutex_ );

        if ( stop_flag_ )
            return false;

        queue_.push_back( request );
        condition_.notify_one();

        return true;
    }

    namespace {
        struct stop_data
        {
            bool&               flag;
            boost::mutex&       mutex;
            boost::condition&   cv;
        };

        extern "C" void rack_call_queue_stop( void* stop_d )
        {
            stop_data* sd = static_cast< stop_data* >( stop_d );

            boost::mutex::scoped_lock lock( sd->mutex );
            sd->flag= true;

            sd->cv.notify_all();
        }

        struct wait_data_call
        {
            ruby_land_queue&                  queue;
            boost::unique_lock<boost::mutex>& lock;
        };

        extern "C" VALUE ack_call_queue_wait( void* func_call )
        {
            wait_data_call* cd = static_cast< wait_data_call* >( func_call );
            cd->queue.wait( cd->lock );

            return VALUE();
        }
    }

    void ruby_land_queue::stop()
    {
        stop_data sd = { stop_flag_, mutex_, condition_ };
        rack_call_queue_stop( &sd );
    }

    void ruby_land_queue::wait( boost::unique_lock< boost::mutex >& lock )
    {
        while ( !stop_flag_ && queue_.empty() )
            condition_.wait( lock );
    }

    void ruby_land_queue::process_request( application_interface& application)
    {
        boost::mutex::scoped_lock lock( mutex_ );

        wait_data_call  waitdata  = { *this, lock };
        stop_data       stopdata  = { stop_flag_, mutex_, condition_ };

        while ( !stop_flag_ )
        {
            rb_thread_blocking_region( &ack_call_queue_wait, &waitdata, &rack_call_queue_stop, &stopdata );

            while ( !stop_flag_ && !queue_.empty() )
            {
                const call_back_t current = queue_.front();
                queue_.pop_front();

                boost::reverse_lock< boost::mutex::scoped_lock > unlock( lock );

                current( application );
            }
        }
    }
}
