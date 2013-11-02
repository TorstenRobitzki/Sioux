#ifndef SIOUX_SOURCE_ASIO_MOCKS_RUN_H
#define SIOUX_SOURCE_ASIO_MOCKS_RUN_H

#include "asio_mocks/test_socket.h"
#include "asio_mocks/test_timer.h"
#include "json/json.h"
#include "http/response.h"
#include "http/decode_stream.h"
#include "server/connection.h"
#include "tools/io_service.h"
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <vector>

namespace asio_mocks
{
    /**
     * @brief structure to keep a http response
     */
    struct response_t
    {
        boost::shared_ptr< http::response_header >  header;
        std::vector< char >                         body;
        boost::posix_time::ptime                    received;
    };

    /**
     * @brief run the simulation
     *
     * The function will run a simulation for at maximum timeout. If no othere IOs where queued during the simulation,
     * the simulation will end before the timeout exceeded. The simulation starts at asio_mocks::current_time().
     *
     * The simulation expects that the socket type is instanciated with asio_mocks::timer as timer.
     *
     * @param timeout the maximum time, the simulation is running
     * @param socket the socket on which to run the simulation
     * @param traits the traits object.
     *
     * @return a list of received responses
     */
    template < class Socket, class Traits >
    std::vector< response_t > run( const boost::posix_time::time_duration& timeout, Socket& socket, Traits& traits );


    namespace details
    {
        class stream_decoder
        {
        public:
            stream_decoder();
            void operator()( boost::asio::const_buffer data );
            std::vector< response_t > result() const;
        private:
            boost::asio::const_buffer feed_data( const boost::asio::const_buffer& data );

            http::stream_decoder< http::response_header >   decoder_;
            std::vector< response_t >                       result_;
            bool                                            idle_;
        };

        void empty_call_back( const boost::system::error_code& );
    }

    template < class Socket, class Traits >
    std::vector< response_t > run( const boost::posix_time::time_duration& timeout, Socket& socket, Traits& traits )
    {
        details::stream_decoder  decoder;
        boost::asio::io_service& queue = socket.get_io_service();

        socket.write_callback( boost::bind< void >( boost::ref( decoder ), _1 ) );

        typedef server::connection< Traits, Socket, asio_mocks::timer > connection_t;
        boost::shared_ptr< connection_t > connection( new connection_t( socket, traits ) );
        connection->start();

        const boost::posix_time::ptime end_of_test = asio_mocks::current_time() + timeout;

        // to wake up at the timeout time during simulation, a timer is scheduled
        asio_mocks::timer timer( queue );
        timer.expires_at( end_of_test );
        timer.async_wait( &details::empty_call_back );

        // in case that the test-setup didn't posted any handler, run() might block
        queue.post( boost::bind( &details::empty_call_back, boost::system::error_code() ) );

        do
        {
            try
            {
                tools::run( queue );
            }
            catch ( const std::exception& e )
            {
                std::cerr << "error running simulation: " << e.what() << std::endl;
            }
            catch ( ... )
            {
                std::cerr << "Unknown error running simulation." << std::endl;
            }
        }
        while ( asio_mocks::current_time() < end_of_test && asio_mocks::advance_time() != 0
            && asio_mocks::current_time() <= end_of_test );

        return decoder.result();
    }
}

#endif


