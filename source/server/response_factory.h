// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SERVER_RESPONSE_FACTORY_H_
#define SIOUX_SERVER_RESPONSE_FACTORY_H_

#include "http/http.h"
#include "http/request.h"
#include "server/error.h"
#include "server/fitting_uri.h"
#include "tools/substring.h"
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/function.hpp>
#include <boost/type_traits/is_same.hpp>

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

        typedef std::vector< std::pair< std::string, boost::shared_ptr< action_holder_base > > > action_list_t;

        action_list_t actions_;

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

        const typename action_list_t::const_iterator action =
            std::find_if( actions_.begin(), actions_.end(), fitting_uri( header->uri() ) );

        if ( action != actions_.end() )
            return (*action->second)( connection, header );

        return error_response( connection, http::http_not_found );
    }

    template < class Socket >
    template <class Connection>
    boost::shared_ptr<async_response> response_factory< Socket >::error_response(const boost::shared_ptr<Connection>& con, http::http_error_code ec) const
    {
        boost::shared_ptr<async_response> result(new ::server::error_response<Connection>(con, ec));
        return result;
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
        actions_.clear();
    }

} // namespace server

#endif /* SIOUX_SERVER_RESPONSE_FACTORY_H_ */
