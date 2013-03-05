// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_FILE_RESPONSE_H_
#define SIOUX_FILE_RESPONSE_H_

#include "server/response.h"
#include "tools/asstring.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/bind.hpp>
#include <iterator>

namespace file
{
    /**
     * @brief very simple implementation of delivering a local file
     */
    template < class Connection >
    class response :
        public boost::enable_shared_from_this< response< Connection > >,
        public server::async_response
    {
    public:
        response( const boost::shared_ptr< Connection >&, const boost::filesystem::path& file_to_deliver );

        void data_written(
            const boost::system::error_code&    error,
            std::size_t                         bytes_transferred);

    private:
        virtual void start();
        virtual const char* name() const;

        const boost::shared_ptr< Connection >       connection_;
        const boost::filesystem::path               path_;
        std::vector< char >                         buffer_;
        std::string                                 header_;
        std::vector< boost::asio::const_buffer >    result_;

        typedef server::report_error_guard< Connection > response_guard;
    };

    // implementation
    template < class Connection >
    response< Connection >::response( const boost::shared_ptr< Connection >& connection,
                                      const boost::filesystem::path&         file_to_deliver )
        : connection_( connection )
        , path_( file_to_deliver )
        , buffer_()
        , header_()
        , result_()
    {
    }

    template < class Connection >
    void response< Connection >::data_written(
        const boost::system::error_code&    error,
        std::size_t                         bytes_transferred)
    {
        if ( !error )
        {
            connection_->response_completed( *this );
        }
        else
        {
            connection_->response_not_possible( *this );
        }
    }

    template < class Connection >
    void response< Connection >::start()
    {
        static std::string response_header =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: ";

        response_guard guard( *connection_, *this, http::http_not_found );

        try 
        {
            boost::filesystem::ifstream input( path_ );

            if ( input.is_open() )
            {
                std::istreambuf_iterator< char > begin( input ), end;
                buffer_.insert( buffer_.begin(), begin, end );

                if ( !input.bad() )
                {
                    header_ = response_header + tools::as_string( buffer_.size() ) + "\r\n\r\n";

                    result_.push_back( boost::asio::buffer( header_ ) );
                    result_.push_back( boost::asio::buffer( buffer_ ) );

                    connection_->async_write(
                        result_,
                        boost::bind( &response::data_written, this->shared_from_this(), _1, _2 ),
                        *this );

                    guard.dismiss();
                }
            }
        }
        catch ( ... ) // error reported by http error code
        {
            /// @todo add loging
        }
    }

    template < class Connection >
    const char* response< Connection >::name() const
    {
        return "file::response";
    }
}

#endif /* SIOUX_FILE_RESPONSE_H_ */
