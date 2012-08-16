// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SERVER_RESPONSE_FACTORY_H_
#define SIOUX_SERVER_RESPONSE_FACTORY_H_

#include "http/http.h"
#include "server/ip_proxy.h"
#include "server/error.h"
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/function.hpp>

namespace http
{
    class request_header;
}

namespace server
{
    class async_response;

    /**
     * @brief a predefined response factory able to create user action responses and proxy responses
     */
    template < class Socket >
    class response_factory : public boost::noncopyable
    {
    public:
        response_factory();

        template < class T >
        explicit response_factory( const T& );

        template < class Connection >
        boost::shared_ptr< async_response > create_response(
            const boost::shared_ptr< Connection >&                    connection,
            const boost::shared_ptr< const http::request_header >&    header);

        template <class Connection>
        boost::shared_ptr<async_response> error_response(const boost::shared_ptr<Connection>& con, http::http_error_code ec) const;

        void add_proxy( boost::asio::io_service& q, const std::string& route, const boost::asio::ip::tcp::endpoint& orgin, const proxy_configuration& c );

        template < class Action >
        void add_action( const std::string& route, const Action& action );

        void shutdown();
    private:
        struct action_holder_base
        {
            template < class Connection >
            boost::shared_ptr<async_response> operator()(
                const boost::shared_ptr< Connection >&                  con,
                const boost::shared_ptr<const http::request_header>&    header ) const
            {
                typedef boost::function< boost::shared_ptr< async_response > (
                    const boost::shared_ptr< Connection >&,
                    const boost::shared_ptr<const http::request_header>& ) > action_t;

                return static_cast< const action_holder< action_t >& >( *this ).call( con, header );
            }

            virtual ~action_holder_base() {}
        };

        template < class Action >
        struct action_holder : action_holder_base
        {
            const Action action_;

            explicit action_holder( const Action& a ) : action_( a )
            {
            }

            template < class Connection >
            boost::shared_ptr<async_response> call(
                const boost::shared_ptr< Connection >&                  con,
                const boost::shared_ptr<const http::request_header>&    header ) const
            {
                return action_( con, header );
            }
        };

        typedef std::vector< std::pair< std::string, boost::shared_ptr< ip_proxy< Socket > > > > proxy_list_t;

        proxy_list_t proxies_;

        typedef std::vector< std::pair< std::string, boost::shared_ptr< action_holder_base > > > action_list_t;

        action_list_t actions_;

        struct fitting_uri
        {
            explicit fitting_uri( const tools::substring& s ) : uri_( s ) {}

            template < class P >
            bool operator()( const P& pair ) const
            {
                const std::size_t s = std::min( uri_.size(), pair.first.size() );

                if ( s != pair.first.size() )
                    return false;

                const tools::substring a( uri_.begin(), uri_.begin() + s );
                const tools::substring b( pair.first.c_str(), pair.first.c_str() + s );

                return a == b;
            }

            const tools::substring  uri_;
        };
    };


    // implementation
    template < class Socket >
    response_factory< Socket >::response_factory()
    {
    }

    template < class Socket >
    template < class T >
    response_factory< Socket >::response_factory( const T& )
    {
    }

    template < class Socket >
    template < class Connection >
    boost::shared_ptr< async_response > response_factory< Socket >::create_response(
        const boost::shared_ptr< Connection >&                    connection,
        const boost::shared_ptr< const http::request_header >&    header)
    {
        BOOST_STATIC_ASSERT( ( boost::is_same< Socket, typename Connection::socket_t >::value ) );

        if ( header->state() != http::message::ok )
            return error_response( connection, http::http_bad_request );

        boost::shared_ptr< async_response > result;

        const typename proxy_list_t::const_iterator proxy =
            std::find_if( proxies_.begin(), proxies_.end(), fitting_uri( header->uri() ) );

        if ( proxy != proxies_.end() )
            result = proxy->second->create_response(connection, header);

        if ( result.get() )
            return result;

        const typename action_list_t::const_iterator action =
            std::find_if( actions_.begin(), actions_.end(), fitting_uri( header->uri() ) );

        if ( action != actions_.end() )
            result = (*action->second)( connection, header );

        return result.get() ? result : error_response( connection, http::http_not_found );
    }

    template < class Socket >
    template <class Connection>
    boost::shared_ptr<async_response> response_factory< Socket >::error_response(const boost::shared_ptr<Connection>& con, http::http_error_code ec) const
    {
        boost::shared_ptr<async_response> result(new ::server::error_response<Connection>(con, ec));
        return result;
    }

    template < class Socket >
    void response_factory< Socket >::add_proxy(boost::asio::io_service& q, const std::string& route, const boost::asio::ip::tcp::endpoint& orgin, const proxy_configuration& c)
    {
        boost::shared_ptr<ip_proxy<Socket> > proxy(
            new ip_proxy<Socket>(q, boost::shared_ptr<const proxy_configuration>(new proxy_configuration(c)), orgin));

        proxies_.push_back(std::make_pair(route, proxy));
    }

    template < class Socket >
    template < class Action >
    void response_factory< Socket >::add_action( const std::string& route, const Action& action )
    {
        actions_.push_back(
            std::make_pair(
                route,
                boost::shared_ptr< action_holder_base >( new action_holder< Action >( action) ) ) );
    }

    template < class Socket >
    void response_factory< Socket >::shutdown()
    {
        proxies_.clear();
        actions_.clear();
    }

} // namespace server

#endif /* SIOUX_SERVER_RESPONSE_FACTORY_H_ */
