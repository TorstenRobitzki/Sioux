// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_PROXY_CONNECTOR_H
#define SIOUX_SOURCE_SERVER_PROXY_CONNECTOR_H

#include "server/proxy.h"
#include "server/error_code.h"
#include "tools/asstring.h"
#include "tools/scope_guard.h"
#include "http/response.h"
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>
#include <stdexcept>
#include <deque>

namespace server
{
    /**
     * @brief will be thrown, if a proxy can not fulify a connection request due to configured limitations
     */
    class proxy_connection_limit_reached : public std::runtime_error
    {
    public:
        proxy_connection_limit_reached(const std::string&);
    };

    class proxy_configuration
    {
    public:
        proxy_configuration();

        // maximum number of connections to an orgin server
        unsigned max_connections() const;
        void max_connections(unsigned);

        // the maximum time a connection to an orgin server stays idle before it will be closed
        // A connection is idle, if it is not used, to communicate with the orgin server, but is connected
        // to the orgin server.
        boost::posix_time::time_duration max_idle_time() const;
        void max_idle_time(const boost::posix_time::time_duration&);

        // Timeout while trying to connect the orgin server
        boost::posix_time::time_duration connect_timeout() const;
        void connect_timeout(const boost::posix_time::time_duration&);

        // Timeout while communicating with the orgin server (both reading and writing)
        boost::posix_time::time_duration orgin_timeout() const;
        void orgin_timeout(const boost::posix_time::time_duration&);

    private:
        unsigned                            max_connections_;
        boost::posix_time::time_duration    max_idle_time_;
        boost::posix_time::time_duration    connect_timeout_;
        boost::posix_time::time_duration    orgin_timeout_;
    };

    /**
     * @brief helper to set configurations on a proxy_configuration
     */
    class proxy_configurator 
    {
    public:
        proxy_configurator();

        const proxy_configurator& max_connections(unsigned) const;
        const proxy_configurator& max_idle_time(const boost::posix_time::time_duration&) const;
        const proxy_configurator& connect_timeout(const boost::posix_time::time_duration&) const;
        const proxy_configurator& orgin_timeout(const boost::posix_time::time_duration&) const;

        boost::shared_ptr<proxy_configuration> config_;

        operator const proxy_configuration&() const;
    };

    /**
     * @brief ip-address/port based proxy_connector
     */
    template <class Socket>
    class ip_proxy_connector : 
        public proxy_connector_base, 
        public boost::enable_shared_from_this<ip_proxy_connector<Socket> >
    {
    public:
        ip_proxy_connector(
            boost::asio::io_service& queue, 
            const boost::shared_ptr<const proxy_configuration>& config, 
            const boost::asio::ip::tcp::endpoint& ep);

        ip_proxy_connector(
            boost::asio::io_service&                queue, 
            const proxy_configuration&              config, 
            const boost::asio::ip::tcp::endpoint&   ep);

        ~ip_proxy_connector();

    private:
        /**
         * @exception proxy_connection_limit_reached
         */
        virtual void async_get_proxy_connection(
            const tools::dynamic_type&,
            const tools::substring&,
            unsigned,
            const boost::shared_ptr<proxy_connector_base::connect_callback>&  call_back);

        virtual void release_connection(
            const tools::dynamic_type&,
            void*                               connection,
            const http::response_header*        header);

        struct connection
        {
            connection(boost::asio::io_service& q, const boost::shared_ptr<proxy_connector_base::connect_callback>& cb)
                : socket_(q)
                , timer_(q)
                , connect_call_back_(cb)
            {
            }
        
            Socket                                                      socket_;
            boost::asio::deadline_timer                                 timer_;
            boost::shared_ptr<proxy_connector_base::connect_callback>   connect_call_back_;
        };

        typedef std::deque<boost::shared_ptr<connection> >  connection_list_t;
        
        void connection_handler(const boost::shared_ptr<connection>&, const boost::system::error_code& error);
        void connect_timeout(const boost::shared_ptr<connection>&, const boost::system::error_code& error);
        void connect_idle_timeout(const boost::shared_ptr<connection>&, const boost::system::error_code& error);
        
        static void remove_from_list(connection_list_t&, typename connection_list_t::iterator);

        boost::mutex                                        mutex_;
        boost::asio::io_service&                            queue_;
        const boost::shared_ptr<const proxy_configuration>  config_;
        const boost::asio::ip::tcp::endpoint                addr_;

        connection_list_t                                   idle_connections_;
        connection_list_t                                   connecting_connections_;
        connection_list_t                                   in_use_connections_;
    };

    ////////////////////////////////////
    // implementation
    template <class Socket>
    ip_proxy_connector<Socket>::ip_proxy_connector(
        boost::asio::io_service& queue, 
        const boost::shared_ptr<const proxy_configuration>& config, 
        const boost::asio::ip::tcp::endpoint& ep)
      : mutex_()
      , queue_(queue)
      , config_(config)
      , addr_(ep)
      , idle_connections_()
      , connecting_connections_()
      , in_use_connections_()
    {
    }

    template <class Socket>
    ip_proxy_connector<Socket>::ip_proxy_connector(
        boost::asio::io_service&                queue, 
        const proxy_configuration&              config, 
        const boost::asio::ip::tcp::endpoint&   ep)
      : mutex_()
      , queue_(queue)
      , config_(new proxy_configuration(config))
      , addr_(ep)
      , idle_connections_()
      , in_use_connections_()
    {
    }

    template <class Socket>
    ip_proxy_connector<Socket>::~ip_proxy_connector()
    {
        assert(in_use_connections_.empty());
    }

    template <class Socket>
    void ip_proxy_connector<Socket>::async_get_proxy_connection(
            const tools::dynamic_type&,
            const tools::substring&,
            unsigned,
            const boost::shared_ptr<proxy_connector_base::connect_callback>&  call_back)
    {
        boost::mutex::scoped_lock lock(mutex_);

        if ( idle_connections_.size() + in_use_connections_.size() + connecting_connections_.size()
            >= config_->max_connections() )
        {
            throw proxy_connection_limit_reached(
                "while connecting to " + tools::as_string(addr_) 
              + "; limit of " + tools::as_string(config_->max_connections()) + " connections reached.");
        }

        if ( idle_connections_.empty() )
        {
            // create connection object
            boost::shared_ptr<connection> new_connection(new connection(queue_, call_back));
            connecting_connections_.push_back(new_connection);
            tools::scope_guard remove_connection = tools::make_obj_guard(connecting_connections_, &connection_list_t::pop_back);

            // connect to peer and setup timeout
            new_connection->socket_.async_connect(
                addr_, 
                boost::bind(
                    &ip_proxy_connector::connection_handler, this->shared_from_this(), 
                    new_connection, _1));

            tools::scope_guard stop_connecting = tools::make_obj_guard(
                new_connection->socket_, 
                static_cast<void(Socket::*)()>(&Socket::close));

            new_connection->timer_.expires_from_now(
                config_->connect_timeout());
            new_connection->timer_.async_wait(
                boost::bind(
                &ip_proxy_connector::connect_timeout, this->shared_from_this(),
                new_connection, _1));

            stop_connecting.dismiss();
            remove_connection.dismiss();
        }
        else // use an already connected connection
        {
            boost::shared_ptr<connection> old_connection = idle_connections_.front();
            idle_connections_.pop_front();

            in_use_connections_.push_back(old_connection);
            tools::scope_guard remove_connection = tools::make_obj_guard(connecting_connections_, &connection_list_t::pop_back);

            old_connection->timer_.cancel();
            queue_.post(boost::bind(&proxy_connector_base::connect_callback::connection_received,
                call_back, &*old_connection, make_error_code(boost::system::errc::success)));

            remove_connection.dismiss();
        }
    }

    namespace {
        template <class L>
        typename L::value_type find_and_remove(L& list, void* value)
        {
            for ( typename L::iterator i = list.begin(); ; ++i )
            {
                assert(i != list.end());
                if ( &**i == value )
                {
                    typename L::value_type result = *i;
                    list.erase(i);

                    return result;
                }
            }
        }
    }

    template <class Socket>
    void ip_proxy_connector<Socket>::release_connection(
            const tools::dynamic_type&,
            void*                               con,
            const http::response_header*        header)
    {
        boost::mutex::scoped_lock lock(mutex_);

        boost::shared_ptr<connection> in_use;
        in_use = find_and_remove(in_use_connections_, con);

        if ( header && !header->option_available("connection", "close") )
        {
            in_use->timer_.expires_from_now(config_->max_idle_time());
            in_use->timer_.async_wait(
                boost::bind(
                    &ip_proxy_connector::connect_idle_timeout, this->shared_from_this(),
                    in_use, _1));

            idle_connections_.push_back(in_use);
        }
    }

    template <class Socket>
    void ip_proxy_connector<Socket>::connection_handler(const boost::shared_ptr<connection>& new_connection, const boost::system::error_code& error)
    {
        boost::shared_ptr<proxy_connector_base::connect_callback>   connect_call_back;
        void* connection = 0;

        {
            boost::mutex::scoped_lock lock(mutex_);
        
            const typename connection_list_t::iterator new_con_pos = 
                std::find(connecting_connections_.begin(), connecting_connections_.end(), new_connection);

            // if the new connection is not found, the connect attempt must have been timed out
            if ( new_con_pos != connecting_connections_.end() )
            {
                tools::scope_guard remove_con_from_list = 
                    tools::make_guard(&ip_proxy_connector::remove_from_list, boost::ref(connecting_connections_), new_con_pos);
                static_cast<void>(remove_con_from_list);

                new_connection->connect_call_back_.swap(connect_call_back);
                assert(connect_call_back.get());
                new_connection->timer_.cancel();

                if ( !error )
                {
                    in_use_connections_.push_back(*new_con_pos);
                    connection = &(*new_con_pos)->socket_;
                }
            }
        }

        if ( connect_call_back.get() )
            connect_call_back->connection_received(connection, error);
    }

    template <class Socket>
    void ip_proxy_connector<Socket>::connect_timeout(
        const boost::shared_ptr<connection>&    timed_out_connection, 
        const boost::system::error_code&        error)
    {
        if ( error ==  boost::asio::error::operation_aborted )
            return;

        boost::shared_ptr<proxy_connector_base::connect_callback>   connect_call_back;

        {
            boost::mutex::scoped_lock lock(mutex_);

            const typename connection_list_t::iterator timed_out_con_pos = 
                std::find(connecting_connections_.begin(), connecting_connections_.end(), timed_out_connection);

            // if the connect was established, while this function was waiting for the mutex, the connection will not 
            // be in the list and everything will be fine.
            if ( timed_out_con_pos != connecting_connections_.end() )
            {
                timed_out_connection->connect_call_back_.swap(connect_call_back);
                connecting_connections_.erase(timed_out_con_pos);
            }
        }

        if ( connect_call_back.get() )
        {
            connect_call_back->connection_received(0, make_error_code(time_out));
        }
    }

    template <class Socket>
    void ip_proxy_connector<Socket>::connect_idle_timeout(
        const boost::shared_ptr<connection>&    timed_out_connection, 
        const boost::system::error_code&        error )
    {
        if ( error ==  boost::asio::error::operation_aborted )
            return;

        boost::shared_ptr<connection> con;

        {
            boost::mutex::scoped_lock lock(mutex_);

            const typename connection_list_t::iterator new_con_pos = 
                std::find(idle_connections_.begin(), idle_connections_.end(), timed_out_connection);

            // if the connect was established, while this function was waiting for the mutex, the connection will not 
            // be in the list and everything will be fine.
            if ( new_con_pos != idle_connections_.end() )
            {
                con.swap(*new_con_pos);
                idle_connections_.erase(new_con_pos);
            }
        }

        if ( con.get() )
        {                            
            boost::system::error_code ec;
            con->socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            con->socket_.close(ec);
        }
    }

    template <class Socket>
    void ip_proxy_connector<Socket>::remove_from_list(connection_list_t& list, typename connection_list_t::iterator pos)
    {
        list.erase(pos);
    }

}

#endif // include guard

