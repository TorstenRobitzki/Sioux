// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_SERVER_H
#define SIOUX_SOURCE_SERVER_SERVER_H

#include "server/traits.h"
#include "server/ip_proxy.h"
#include "server/connection.h"
#include "server/error.h"
#include "server/log.h"
#include <boost/asio/io_service.hpp>
#include <boost/thread/thread.hpp>

namespace server {

    template <class Socket>
    class response_factory
    {
    public:
        response_factory()
        {
        }

        template <class T>
        explicit response_factory(const T&)
        {
        }

        template <class Connection>
        boost::shared_ptr<async_response> create_response(
            const boost::shared_ptr<Connection>&                    connection,
            const boost::shared_ptr<const http::request_header>&    header)
        {
            std::cout << header->uri() << std::endl;
            if ( proxies_.size() == 1 )
            {
                return proxies_.front().second->create_response(connection, header);
            }

            typename proxy_list_t::const_iterator proxy = proxies_.begin();
            for ( ; proxy != proxies_.end(); ++proxy )
            {
                const std::size_t s = std::min( header->uri().size(), proxy->first.size());
 
                if ( s == proxy->first.size() )
                {
                    const tools::substring a(header->uri().begin(), header->uri().begin()+s);
                    const tools::substring b(proxy->first.c_str(), proxy->first.c_str()+s);

                    if ( a == b )
                        return proxy->second->create_response(connection, header);
                }
            }

            return boost::shared_ptr<async_response>();
        }

        template <class Connection>
        boost::shared_ptr<async_response> error_response(const boost::shared_ptr<Connection>& con, http::http_error_code ec) const
        {
            boost::shared_ptr<async_response> result(new ::server::error_response<Connection>(con, ec));
            return result;
        }

        void add_proxy(boost::asio::io_service& q, const std::string& route, const boost::asio::ip::tcp::endpoint& orgin, const proxy_configuration& c)
        {
            boost::shared_ptr<ip_proxy<Socket> > proxy(
                new ip_proxy<Socket>(q, boost::shared_ptr<const proxy_configuration>(new proxy_configuration(c)), orgin));

            proxies_.push_back(std::make_pair(route, proxy));
        }
    private:
        response_factory(const response_factory&);
        response_factory& operator=(const response_factory&);

        typedef std::vector<std::pair<std::string, boost::shared_ptr<ip_proxy<Socket> > > > proxy_list_t;
    
        proxy_list_t proxies_;
    };

    /**
     * @brief accepts incoming connection and creates connection objects from that
     */
    template <class Trait, class Connection>
    class acceptator
    {
    public:
        acceptator(boost::asio::io_service& s, Trait& trait, const boost::asio::ip::tcp::endpoint& ep)
            : end_point_(ep)
            , acceptor_(s, ep)
            , queue_(s)
            , connection_()
            , trait_(trait)
        {
            issue_accept();
        }

    private:
        typedef connection<Trait, Connection> connection_t;

        void issue_accept()
        {
            connection_.reset(new connection_t(boost::ref(queue_), trait_));
            acceptor_.async_accept(connection_->socket(), end_point_, boost::bind(&acceptator::handler_connect, this, boost::asio::placeholders::error));
        }

        void handler_connect(const boost::system::error_code& error)
        {
            if ( !error )
            {
                connection_->start();
                issue_accept();
            }
            else
            {
                std::cerr << "handler: " << error << std::endl;
            }
        }

        boost::asio::ip::tcp::endpoint  end_point_;
        boost::asio::ip::tcp::acceptor  acceptor_;
        boost::asio::io_service&        queue_;
        boost::shared_ptr<connection_t> connection_;
        Trait&                          trait_;
    };

    /**
     * @brief a http server, listening on given ports
     */
    template <class Trait>
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

        void add_listener(const boost::asio::ip::tcp::endpoint&);
        void add_proxy(const char* route, const boost::asio::ip::tcp::endpoint& orgin, const proxy_configuration&);
    private:
        typedef boost::asio::ip::tcp::socket    socket_t;
        typedef Trait                           trait_t;
        typedef acceptator<trait_t, socket_t>   acceptor_t;

        boost::asio::io_service&                    queue_;
        std::vector<boost::shared_ptr<acceptor_t> > acceptors_;
        trait_t                                     trait_;
        boost::thread_group                         thread_herd_;
    };
    
    typedef basic_server<connection_traits<boost::asio::ip::tcp::socket, response_factory<boost::asio::ip::tcp::socket> > > http_server;

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

    template <class EventLog = stream_event_log, class ErrorLog = stream_error_log>
    class logging_server : 
        public details::stream_ref_holder,
        public basic_server<connection_traits<boost::asio::ip::tcp::socket, response_factory<boost::asio::ip::tcp::socket>, EventLog,  ErrorLog> >
    {
    public:
        logging_server(boost::asio::io_service& queue, unsigned number_of_threads, std::ostream& out)
            : details::stream_ref_holder(out)
            , basic_server<connection_traits<boost::asio::ip::tcp::socket, response_factory<boost::asio::ip::tcp::socket>, EventLog, ErrorLog> >(
                queue, number_of_threads, *this)
        {
        }
    };

    ///////////////////////
    // imeplementation
    template <class Trait>
    basic_server<Trait>::basic_server(boost::asio::io_service& queue, unsigned number_of_threads)
        : queue_(queue)
        , acceptors_()
        , trait_()
        , thread_herd_()
    {
        for (; number_of_threads; --number_of_threads)
            thread_herd_.create_thread(boost::bind(&boost::asio::io_service::run, &queue));
    }

    template <class Trait>
    template <class TraitParameters>
    basic_server<Trait>::basic_server(boost::asio::io_service& queue, unsigned number_of_threads, const TraitParameters& param)
        : queue_(queue)
        , acceptors_()
        , trait_(param)
        , thread_herd_()
    {
        for (; number_of_threads; --number_of_threads)
            thread_herd_.create_thread(boost::bind(&boost::asio::io_service::run, &queue));
    }

    template <class Trait>
    basic_server<Trait>::~basic_server()
    {
        thread_herd_.join_all();
    }

    template <class Trait>
    void basic_server<Trait>::add_listener(const boost::asio::ip::tcp::endpoint& ep)
    {
        boost::shared_ptr<acceptor_t> accept(new acceptor_t(queue_, trait_, ep));
        acceptors_.push_back(accept);
    }

    template <class Trait>
    void basic_server<Trait>::add_proxy(const char* route, const boost::asio::ip::tcp::endpoint& orgin, const proxy_configuration& config)
    {
        trait_.add_proxy(queue_, route, orgin, config);
    }

} // namespace server

#endif // include guard



