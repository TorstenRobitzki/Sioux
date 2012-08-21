// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_SERVER_H
#define SIOUX_SOURCE_SERVER_SERVER_H

#include "server/traits.h"
#include "server/connection.h"
#include "server/log.h"
#include "server/response_factory.h"
#include <boost/asio/io_service.hpp>
#include <boost/thread/thread.hpp>
#include <boost/function.hpp>
#include <boost/utility.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>

namespace server {


    /**
     * @brief accepts incoming connection and creates connection objects from that
     */
    template < class Trait, class Connection >
    class acceptator : public boost::enable_shared_from_this< acceptator< Trait, Connection > >
    {
    public:
        acceptator(boost::asio::io_service& s, Trait& trait, const boost::asio::ip::tcp::endpoint& ep)
            : end_point_(ep)
            , acceptor_(s, ep)
            , queue_(s)
            , trait_(trait)
            , timer_( queue_ )
        {
        }

        void start()
        {
            issue_accept();
        }

        void shut_down()
        {
            boost::system::error_code ec;
            timer_.cancel( ec );
            acceptor_.cancel( ec );
        }

    private:
        typedef connection<Trait, Connection> connection_t;

        void issue_accept()
        {
            boost::shared_ptr<connection_t> connection( new connection_t( boost::ref( queue_ ), trait_ ) );

            boost::function< void(boost::system::error_code) > f =
                boost::bind( &acceptator::handler_connect, this->shared_from_this(), connection, _1 );

            acceptor_.async_accept( connection->socket(), end_point_, f );
        }

        void handler_connect( boost::shared_ptr<connection_t> connection, boost::system::error_code& error )
        {
            if ( !error )
            {
                connection->trait().event_accepting_new_connection( end_point_, connection->socket().remote_endpoint() );

                connection->start();
                issue_accept();
            }
            else if ( error != boost::asio::error::operation_aborted )
            {
                trait_.error_accepting_new_connection( end_point_, error );

                timer_.expires_from_now( trait_.reaccept_timeout() );
                timer_.async_wait(
                    boost::bind( &acceptator::handle_reaccept_timeout, this->shared_from_this(), boost::asio::placeholders::error ) );
            }
        }

        void handle_reaccept_timeout( const boost::system::error_code& ec )
        {
            if ( ec )
                return;

            issue_accept();
        }

        boost::asio::ip::tcp::endpoint  end_point_;
        boost::asio::ip::tcp::acceptor  acceptor_;
        boost::asio::io_service&        queue_;
        Trait&                          trait_;
        boost::asio::deadline_timer     timer_;
    };

    /**
     * @brief a http server, listening on given ports
     */
    template < class Trait >
    class basic_server
    {
    public:
        /**
         * @brief constructs a new server and starts the given number of threads to run the given queue
         *
         * 0 is a valid value for number_of_threads. In that case, the queue have to be run from some where else.
         */
        basic_server(boost::asio::io_service& queue, unsigned number_of_threads);

        /**
         * @brief constructs a new server and starts the given number of threads to run the given queue
         *
         * 0 is a valid value for number_of_threads. In that case, the queue have to be run from some where else.
         */
        template <class TraitParameters>
        basic_server(boost::asio::io_service& queue, unsigned number_of_threads, const TraitParameters& param);

        virtual ~basic_server();

        /**
         * @brief adds a new tcp::endpoint where the the server will listen for incomming connections
         */
        void add_listener( const boost::asio::ip::tcp::endpoint& );

        /**
         * @brief adds a new route to an orgin server
         */
        void add_proxy( const char* route, const boost::asio::ip::tcp::endpoint& orgin, const proxy_configuration&);

        /**
         * a function taking a connection and a request_header, returning a async_response
         */
        typedef boost::function< boost::shared_ptr< async_response > (
            const boost::shared_ptr< connection< Trait > >&,
            const boost::shared_ptr<const http::request_header>& ) > action_t;

        /**
         * @brief adds a new route for a user defined action
         */
        void add_action( const char* route, const action_t& action );

        /**
         * @brief stops accepting incomming connections, close all listen ports
         *
         * The function is need as the io_service give to the c'tor needs to be destructed before the server.
         * So basically this function must make sure, that the passed reference isn't used anymore.
         */
        void shut_down();

        /**
         * @brief joins the thread pool
         */
        void join();

        typedef Trait                           trait_t;
        trait_t& trait();

        typedef connection< Trait >             connection_t;
    private:
        typedef boost::asio::ip::tcp::socket    socket_t;
        typedef acceptator<trait_t, socket_t>   acceptor_t;
        typedef std::vector< boost::shared_ptr< acceptor_t > >  acceptor_list_t;
        boost::asio::io_service&                    queue_;
        trait_t                                     trait_;
        boost::thread_group                         thread_herd_;

        acceptor_list_t                             acceptors_;
        bool                                        shutting_down_;
    };
    
    typedef basic_server<
        connection_traits<
            boost::asio::ip::tcp::socket,
            boost::asio::deadline_timer,
            response_factory< boost::asio::ip::tcp::socket > > > http_server;

    namespace details {
        class stream_ref_holder
        {
        public:
            explicit stream_ref_holder(std::ostream& out) : out_(out) {}
            std::ostream& logstream() const { return out_; }
        private:
            std::ostream&   out_;
        };
    }

    /**
     * @brief server implementation with logging enabled
     */
    template <class EventLog = stream_event_log, class ErrorLog = stream_error_log>
    class logging_server : 
        public details::stream_ref_holder,
        public basic_server<
            connection_traits<
                boost::asio::ip::tcp::socket,
                boost::asio::deadline_timer,
                response_factory< boost::asio::ip::tcp::socket >,
                EventLog,
                ErrorLog > >
    {
    public:
        typedef basic_server<
            connection_traits<
                boost::asio::ip::tcp::socket,
                boost::asio::deadline_timer,
                response_factory< boost::asio::ip::tcp::socket >,
                EventLog,
                ErrorLog > > base_t;

        logging_server( boost::asio::io_service& queue, unsigned number_of_threads, std::ostream& out )
            : details::stream_ref_holder(out)
            , base_t( queue, number_of_threads, *this)
        {
        }
    };

    ///////////////////////
    // imeplementation
    template <class Trait>
    basic_server<Trait>::basic_server(boost::asio::io_service& queue, unsigned number_of_threads)
        : queue_(queue)
        , trait_()
        , thread_herd_()
        , acceptors_()
        , shutting_down_( false )
    {
        for (; number_of_threads; --number_of_threads)
            thread_herd_.create_thread(boost::bind(&boost::asio::io_service::run, &queue));
    }

    template <class Trait>
    template <class TraitParameters>
    basic_server<Trait>::basic_server(boost::asio::io_service& queue, unsigned number_of_threads, const TraitParameters& param)
        : queue_(queue)
        , trait_(param)
        , thread_herd_()
        , acceptors_()
        , shutting_down_( false )
    {
        for (; number_of_threads; --number_of_threads)
            thread_herd_.create_thread(boost::bind(&boost::asio::io_service::run, &queue));
    }

    template <class Trait>
    basic_server<Trait>::~basic_server()
    {
        if ( !shutting_down_ )
            shut_down();
    }

    template <class Trait>
    void basic_server<Trait>::add_listener(const boost::asio::ip::tcp::endpoint& ep)
    {
        assert( !shutting_down_ );
        boost::shared_ptr< acceptor_t > accept(new acceptor_t(queue_, trait_, ep));
        acceptors_.push_back(accept);
        accept->start();
    }

    template <class Trait>
    void basic_server<Trait>::add_proxy(const char* route, const boost::asio::ip::tcp::endpoint& orgin, const proxy_configuration& config)
    {
        assert( !shutting_down_ );
        trait_.add_proxy(queue_, route, orgin, config);
    }

    template < class Trait >
    void basic_server<Trait>::add_action( const char* route, const action_t& action )
    {
        assert( !shutting_down_ );
        trait_.add_action( route, action );
    }

    template <class Trait>
    typename basic_server<Trait>::trait_t& basic_server<Trait>::trait()
    {
        return trait_;
    }

    template < class Trait >
    void basic_server<Trait>::shut_down()
    {
        shutting_down_ = true;

        for ( typename acceptor_list_t::iterator acc = acceptors_.begin(); acc != acceptors_.end(); ++acc )
            ( *acc )->shut_down();

        trait_.shutdown();
    }

    template < class Trait >
    void basic_server< Trait >::join()
    {
        thread_herd_.join_all();
    }

} // namespace server

#endif // include guard



