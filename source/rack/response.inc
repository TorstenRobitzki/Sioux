// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "rack/response.h"
#include "http/request.h"
#include <boost/asio/buffer.hpp>
#include <ruby.h>

namespace rack
{
    template < class Connection >
    response< Connection >::response(
        const boost::shared_ptr< Connection >&                  connection,
        const boost::shared_ptr< const http::request_header >&  request,
        boost::asio::io_service&                                queue,
        ruby_land_queue&                                        ruby_land )
        : connection_( connection )
        , request_( request )
        , queue_( queue )
        , ruby_land_queue_( ruby_land )
        , write_ptr_( 0 )
    {
    }

    template < class Connection >
    void response< Connection >::start()
    {
        server::close_connection_guard< connection_t >  error_guard( *connection_, *this );

        // read the body if existing
        if ( request_->body_expected() )
        {
            connection_->async_read_body(
                boost::bind( &response::body_read_handler, this->shared_from_this(), _1, _2, _3 ) );

            error_guard.dismiss();
        }
        else
        {
            if ( ruby_land_queue_.push(
                boost::bind( &response< Connection >::call_application, this->shared_from_this(), _1 ) ) )
            {
                error_guard.dismiss();
            }
        }

    }

    template < class Connection >
    void response< Connection >::body_read_handler(
        const boost::system::error_code& error,
        const char* buffer,
        std::size_t bytes_read_and_decoded )
    {
        server::close_connection_guard< connection_t >  error_guard( *connection_, *this );

        if ( !error )
        {
            body_.insert( body_.begin(), buffer, buffer + bytes_read_and_decoded );

            if ( bytes_read_and_decoded != 0
               || ruby_land_queue_.push(
                    boost::bind( &response< Connection >::call_application, this->shared_from_this(), _1 ) ) )
            {
                error_guard.dismiss();
            }
        }
    }

    template < class Connection >
    void response< Connection >::call_application( application_interface& call_back )
    {
        server::close_connection_guard< connection_t >  error_guard( *connection_, *this );

        response_ = call_back.call( body_, *request_ );

        if ( response_.size() == 0 )
            return;

        assert( write_ptr_ < response_.size() );
        connection_->async_write_some(
            boost::asio::buffer( &response_[ write_ptr_ ], response_.size() - write_ptr_ ),
            boost::bind( &response::response_write_handler, this->shared_from_this(), _1, _2 ),
            *this );

        error_guard.dismiss();
    }

    template < class Connection >
    void response< Connection >::response_write_handler( const boost::system::error_code& error, std::size_t bytes_transferred )
    {
        server::close_connection_guard< connection_t >  error_guard( *connection_, *this );

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
}
