// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef PROXY_TEST_TRAITS_H_
#define PROXY_TEST_TRAITS_H_

#include "server/test_socket.h"
#include "server/test_timer.h"
#include "server/test_response.h"
#include "server/error.h"
#include "server/traits.h"
#include "server/log.h"
#include <vector>

namespace server {

    class async_response;

    namespace test {

        class timer;

    }
}

namespace proxy {
    namespace test {

    class connector;

    /**
     * @brief test traits, that uses a test::socket, a test::timer and records the incoming requests and outgoing responses.
     */
    template < class ResponseFactory >
    class traits : public server::connection_traits< server::test::socket< const char* >, server::test::timer, ResponseFactory, server::null_event_logger >
    {
    public:
        traits( connector& p, boost::asio::io_service& io ) : pimpl_( new impl( p, io ) )
        {
        }

        template < class Connection >
        boost::shared_ptr< server::async_response > create_response(
            const boost::shared_ptr< Connection >&                    connection,
            const boost::shared_ptr< const http::request_header >&    header)
        {
            pimpl_->add_request( header );
            const boost::shared_ptr< server::async_response > result(
                ResponseFactory::create_response( connection, header, *this ) );
            pimpl_->add_response( result );

            return result;
        }

        std::vector< boost::shared_ptr< const http::request_header > > requests() const
        {
            return pimpl_->requests();
        }

        template < class Connection >
        boost::shared_ptr< server::async_response > error_response(
            const boost::shared_ptr< Connection >&  con,
            http::http_error_code                   ec ) const
        {
            boost::shared_ptr< server::async_response > result (new server::error_response< Connection >( con, ec ) );
            return result;
        }

        connector& proxy() const
        {
            return pimpl_->proxy();
        }

        boost::asio::io_service& io_queue() const
        {
            return pimpl_->io_queue();
        }

        std::vector< boost::shared_ptr< server::async_response > > responses() const
        {
            return pimpl_->responses();
        }

        void reset_responses()
        {
            pimpl_->reset_responses();
        }

    private:
        class impl
        {
        public:
            explicit impl( connector& p, boost::asio::io_service& i )
                : requests_()
                , proxy_( p )
                , io_( i )
            {
            }

            void add_request( const boost::shared_ptr< const http::request_header >& r )
            {
                requests_.push_back( r );
            }

            void add_response( const boost::shared_ptr< server::async_response >& r )
            {
                responses_.push_back( r );
            }

            std::vector< boost::shared_ptr< const http::request_header > > requests() const
            {
                return requests_;
            }

            std::vector< boost::shared_ptr< server::async_response > > responses() const
            {
                return responses_;
            }

            void reset_responses()
            {
                responses_.clear();
            }

            connector& proxy() const
            {
                return proxy_;
            }

            boost::asio::io_service& io_queue() const
            {
                return io_;
            }

        private:
            std::vector< boost::shared_ptr< const http::request_header > >  requests_;
            std::vector< boost::shared_ptr< server::async_response > >      responses_;
            connector&                                                      proxy_;
            boost::asio::io_service&                                        io_;
        };

        boost::shared_ptr< impl > pimpl_;
    };

} // namespace test
} // namespace proxy


#endif /* PROXY_TEST_TRAITS_H_ */
