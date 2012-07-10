// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "rack/response.h"
#include "http/request.h"
#include <boost/asio/buffer.hpp>
#include <boost/thread/reverse_lock.hpp>
#include <ruby.h>

namespace rack
{
    template < class Connection >
    response< Connection >::response(
        const boost::shared_ptr< connection_t >&                connection,
        const boost::shared_ptr< const http::request_header >&  request,
        boost::asio::io_service&                                queue,
        call_queue< response< Connection > >&                   call_queue)
        : connection_( connection )
        , request_( request )
        , queue_( queue )
        , call_queue_( call_queue )
        , write_ptr_( 0 )
    {
    }

    template < class Connection >
    void response< Connection >::start()
    {
        server::report_error_guard< connection_t >  error_guard( *connection_, *this );

        // read the body if existing
        if ( request_->body_expected() )
        {
            connection_->async_read_body(
                boost::bind( &response::body_read_handler, this->shared_from_this(), _1, _2, _3 ) );
        }
        else
        {
            call_queue_.push( this->shared_from_this() );
        }

        error_guard.dismiss();
    }

    template < class Connection >
    void response< Connection >::body_read_handler(
        const boost::system::error_code& error,
        const char* buffer,
        std::size_t bytes_read_and_decoded )
    {
        server::report_error_guard< connection_t >  error_guard( *connection_, *this );

        if ( !error )
        {
            body_.insert( body_.begin(), buffer, buffer + bytes_read_and_decoded );

            if ( bytes_read_and_decoded == 0 )
            {
                call_queue_.push( this->shared_from_this() );
            }
            else
            {
                connection_->async_read_body(
                    boost::bind( &response::body_read_handler, this->shared_from_this(), _1, _2, _3 ) );
            }

            error_guard.dismiss();
        }
    }

    template < class Connection >
    void response< Connection >::call_application( call_back_interface& call_back )
    {
        response_ = call_back.call( body_, *request_ );
    }

    template < class Connection >
    void response< Connection >::send_response()
    {
        if ( response_.size() == 0 )
            return connection_->response_not_possible( *this );

        try
        {
            assert( write_ptr_ < response_.size() );
            connection_->async_write_some(
                boost::asio::buffer( &response_[ write_ptr_ ], response_.size() - write_ptr_ ),
                boost::bind( &response::response_write_handler, this->shared_from_this(), _1, _2 ),
                *this );
        }
        catch ( ... )
        {
            connection_->response_not_possible( *this );
        }
    }

    template < class Connection >
    void response< Connection >::response_write_handler( const boost::system::error_code& error, std::size_t bytes_transferred )
    {
        server::report_error_guard< connection_t >  error_guard( *connection_, *this );

        if ( !error )
        {
            write_ptr_ += bytes_transferred;
            assert( write_ptr_ <= response_.size() );

            if ( write_ptr_ == response_.size() )
            {
                connection_->response_completed( *this );
            }
            else
            {
                connection_->async_write_some(
                    boost::asio::buffer( &response_[ write_ptr_ ], response_.size() - write_ptr_ ),
                    boost::bind( &response::response_write_handler, this->shared_from_this(), _1, _2 ),
                    *this );
            }

            error_guard.dismiss();
        }
    }


    //////////////////
    // call_queue
    template < class T >
    call_queue< T >::call_queue()
     : mutex_()
     , condition_()
     , stop_flag_( false )
     , queue_()
    {
    }

    template < class T >
    void call_queue< T >::push( const boost::shared_ptr< T >& request )
    {
        boost::mutex::scoped_lock lock( mutex_ );

        if ( stop_flag_ )
            return;

        queue_.push_back( request );
        condition_.notify_one();
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

        template < class T >
        void ack_call_queue_wait2( void* p, boost::unique_lock<boost::mutex>& lock )
        {
            static_cast< T* >( p )->wait( lock );
        }

        struct wait_data_call
        {
            void (*func)(void*, boost::unique_lock<boost::mutex>&);
            void*                             data;
            boost::unique_lock<boost::mutex>& lock;
        };

        extern "C" VALUE ack_call_queue_wait( void* func_call )
        {
            wait_data_call* cd = static_cast< wait_data_call* >( func_call );
            cd->func( cd->data, cd->lock );

            return VALUE();
        }

        template < class M >
        struct unlock_guard
        {

        };
    }

    template < class T >
    void call_queue< T >::stop()
    {
        stop_data sd = { stop_flag_, mutex_, condition_ };
        rack_call_queue_stop( &sd );
    }

    template < class T >
    void call_queue< T >::wait( boost::unique_lock<boost::mutex>& lock )
    {
        while ( !stop_flag_ && queue_.empty() )
            condition_.wait( lock );
    }

    template < class T >
    void call_queue< T >::process_request( call_back_interface& inter )
    {
        boost::mutex::scoped_lock lock( mutex_ );

        wait_data_call               call = { &ack_call_queue_wait2< call_queue< T > >, this, lock };
        stop_data                    stopdata = { stop_flag_, mutex_, condition_ };

        while ( !stop_flag_ )
        {
            rb_thread_blocking_region( &ack_call_queue_wait, &call, &rack_call_queue_stop, &stopdata );

            while ( !stop_flag_ && !queue_.empty() )
            {
                boost::shared_ptr< T > current = queue_.front();
                queue_.pop_front();

                boost::reverse_lock< boost::mutex::scoped_lock > unlock( lock );

                current->call_application( inter );
                current->send_response();
            }
        }
    }

}
