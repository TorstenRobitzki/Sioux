// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_BAYEUX_TEST_TOOLS_H_
#define SIOUX_BAYEUX_TEST_TOOLS_H_

#include <boost/mpl/if.hpp>

#include "bayeux/bayeux.h"
#include "bayeux/configuration.h"
#include "bayeux/adapter.h"
#include "http/request.h"
#include "http/response.h"
#include "pubsub/configuration.h"
#include "pubsub/root.h"
#include "pubsub/test_helper.h"
#include "server/error.h"
#include "server/log.h"
#include "server/test_io_plan.h"
#include "server/test_session_generator.h"
#include "server/test_socket.h"
#include "server/test_timer.h"
#include "server/traits.h"

namespace boost {
    namespace asio {
        class io_service;
    }
}

namespace bayeux
{
    namespace test
    {
        struct response_factory
        {
            template < class Trait >
            explicit response_factory( Trait& trait )
              : bayeux_connector( trait.connector() )
            {
            }

            template < class Connection >
            boost::shared_ptr< server::async_response > create_response(
                const boost::shared_ptr< Connection >&                    connection,
                const boost::shared_ptr< const http::request_header >&    header )
            {
                if ( header->state() == http::message::ok )
                {
                    return bayeux_connector.create_response( connection, header );
                }

                return boost::shared_ptr< server::async_response >(
                        new server::error_response< Connection >( connection, http::http_bad_request ) );
            }

            template < class Connection >
            boost::shared_ptr< server::async_response > error_response(
                const boost::shared_ptr< Connection >&  con,
                http::http_error_code                   ec )
            {
                return boost::shared_ptr< server::async_response >( new ::server::error_response< Connection >( con, ec ) );
            }

            bayeux::connector< server::test::timer >&   bayeux_connector;
        };

        class trait_data
        {
        public:
            trait_data( boost::asio::io_service& queue, pubsub::root& data, const bayeux::configuration& config )
                : session_generator_()
                , connector_( queue, data, session_generator_, config )
            {
            }

            template < class SessionData >
            trait_data( boost::asio::io_service& queue, pubsub::root& data, bayeux::adapter< SessionData >& adapter,
                const bayeux::configuration& config )
                : session_generator_()
                , connector_( queue, data, session_generator_, adapter, config )
            {
            }

            std::ostream& logstream() const
            {
                return std::cerr;
            }

            bayeux::connector< server::test::timer >& connector() const
            {
                return connector_;
            }
        private:
            server::test::session_generator                     session_generator_;
            mutable bayeux::connector< server::test::timer >    connector_;
        };

        typedef server::test::timer         timer_t;
        typedef server::test::socket< const char*, timer_t > socket_t;

        // set to true_ for extended debug messages
        typedef boost::mpl::false_ extended_debugging;

        typedef boost::mpl::if_< extended_debugging, server::stream_event_log, server::null_event_logger >::type
            event_logger_t;

        typedef boost::mpl::if_< extended_debugging, server::stream_error_log, server::null_error_logger >::type
            error_logger_t;

        class trait_t :
            public trait_data,
            public server::connection_traits< socket_t, timer_t, response_factory, event_logger_t, error_logger_t >
        {
        public:
            typedef server::connection_traits< socket_t, timer_t, response_factory, event_logger_t, error_logger_t > base_t;

            trait_t( boost::asio::io_service& queue, pubsub::root& data,
                        const bayeux::configuration& config = bayeux::configuration() )
                : trait_data( queue, data, config )
                , base_t( *this )
            {
            }

            template < class SessionData >
            trait_t( boost::asio::io_service& queue, pubsub::root& data, bayeux::adapter< SessionData >& adapter,
                        const bayeux::configuration& config = bayeux::configuration() )
                : trait_data( queue, data, adapter, config )
                , base_t( *this )
            {
            }
        };

        /*
         * everything that is needed for a complete test
         */
        struct context
        {
            boost::asio::io_service queue;
            pubsub::test::adapter   adapter;
            pubsub::root            data;
            trait_t                 trait;

            context()
                : queue()
                , adapter()
                , data( queue, adapter, pubsub::configuration() )
                , trait( queue, data )
            {
                server::test::reset_time();
            }

            explicit context( const pubsub::configuration& config )
                : queue()
                , adapter()
                , data( queue, adapter, config )
                , trait( queue, data )
            {
                server::test::reset_time();
            }

            context( const pubsub::configuration& config, const bayeux::configuration& bayeux_config )
                : queue()
                , adapter()
                , data( queue, adapter, config )
                , trait( queue, data, bayeux_config )
            {
                server::test::reset_time();
            }

            template < class SessionData >
            explicit context( bayeux::adapter< SessionData >& adapt )
                : queue()
                , adapter()
                , data( queue, adapter, pubsub::configuration() )
                , trait( queue, data, adapt )
            {
                server::test::reset_time();
            }
        };

        struct response_t
        {
            boost::shared_ptr< http::response_header >  first;
            json::array                                 second;
            boost::posix_time::ptime                    received;
        };

        /**
         * creates a http message out of a bayeux body
         */
        server::test::read msg( const std::string& txt );

        /**
         * creates a http message out of a bayeux body
         */
        template < std::size_t S >
        server::test::read msg( const char(&txt)[S] )
        {
            return msg( std::string( txt ) );
        }

        /**
         * Takes the simulated client input, records the response and extracts the bayeux messages from the
         * http responses.
         */
        std::vector< response_t > bayeux_session( const server::test::read_plan& input,
            const server::test::write_plan& output, bayeux::test::context& context,
            const boost::posix_time::time_duration& timeout = boost::posix_time::minutes( 60 ) );

        std::vector< bayeux::test::response_t > bayeux_session( const server::test::read_plan& input,
            bayeux::test::context& context );

        std::vector< bayeux::test::response_t > bayeux_session( const server::test::read_plan& input );

        /**
         * extracts the bayeux messages from the given list of reponses
         */
        json::array bayeux_messages( const std::vector< bayeux::test::response_t >& http_response );

        /**
         * can be used to update a pubsub node within an read_plan
         */
        boost::function< void() > update_node( bayeux::test::context& context, const char* channel_name,
            const json::value& data, const json::value* id = 0 );
    }
}

#endif /* SIOUX_BAYEUX_TEST_TOOLS_H_ */
