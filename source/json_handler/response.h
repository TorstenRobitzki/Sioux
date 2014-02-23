#include "server/response.h"
#include "http/http.h"
#include "http/request.h"
#include "json/json.h"

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/system/error_code.hpp>
#include <boost/function.hpp>

namespace json
{
    /*
     * not template dependend implementation of response_base
     */
    class response_base : public server::async_response
    {
    public:
        /**
         * @brief returns "json::response"
         */
        const char* name() const;

        /**
         * @brief the signature of the handler. A function takeing a request header and a request body and producing a
         *        response body and a response code.
         */
        typedef boost::function<
            std::pair< json::value, http::http_error_code > ( const ::http::request_header& header, const json::value& body )
                > handler_t;

    protected:
        response_base( const boost::shared_ptr< const http::request_header >& request, const handler_t& handler );

        void build_response( const json::value& response_body );

        const boost::shared_ptr< const http::request_header >   request_;
        const handler_t                                         handler_;

        // reading a request body
        json::parser                                            parser_;

        // response informations
        json::value                                             response_body_;
        std::string                                             response_line_;
        std::string                                             body_size_;
        std::vector< boost::asio::const_buffer >                response_;
    };

    /**
     * @brief class, responsible to read a JSON body from a connection, call a given callback and send a JSON encoded
     * message response back.
     *
     * If the receveived request contains no request body, a json::null() is passed a argument to the handler.
     */
    template < class Connection >
    class response :
        public boost::enable_shared_from_this< response< Connection > >,
        public response_base
    {
    public:
        /**
         * @brief responses with the given JSON value and http code 200 (ok)
         */
        response( const boost::shared_ptr< Connection >& con, const boost::shared_ptr< const http::request_header >& request,
            const handler_t& handler );

        /**
         * @brief starts sending the response over the given connection.
         */
        void start();

    private:
        void body_read_handler(
            const boost::system::error_code& error,
            const char* buffer,
            std::size_t bytes_read_and_decoded );

        void response_written( const boost::system::error_code& ec, std::size_t size );

        const boost::shared_ptr< Connection >   connection_;
    };


    // implementation
    template < class Connection >
    response< Connection >::response( const boost::shared_ptr< Connection >& con, const boost::shared_ptr< const http::request_header >& request,
        const handler_t& handler )
        : response_base( request, handler )
        , connection_( con )
    {
    }

    template < class Connection >
    void response< Connection >::body_read_handler(
        const boost::system::error_code& error,
        const char* buffer,
        std::size_t bytes_read_and_decoded )
    {
        server::close_connection_guard< Connection > guard( *connection_, *this );

        if ( !error )
        {
            if ( bytes_read_and_decoded == 0 )
            {
                parser_.flush();
                build_response( parser_.result() );

                connection_->async_write(
                    response_, boost::bind( &response::response_written, this->shared_from_this(), _1, _2 ), *this );
            }
            else
            {
                parser_.parse( buffer, buffer + bytes_read_and_decoded );
            }

            guard.dismiss();
        }
    }

    template < class Connection >
    void response< Connection >::start()
    {
        server::close_connection_guard< Connection > guard( *connection_, *this );

        if ( request_->body_expected() )
        {
            connection_->async_read_body( boost::bind( &response::body_read_handler, this->shared_from_this(), _1, _2, _3 ) );
        }
        else
        {
            build_response( json::null() );

            connection_->async_write(
                response_, boost::bind( &response::response_written, this->shared_from_this(), _1, _2 ), *this );
        }

        guard.dismiss();
    }

    template < class Connection >
    void response< Connection >::response_written( const boost::system::error_code& ec, std::size_t s )
    {
        if ( ec )
        {
            connection_->response_not_possible( *this );
        }
        else
        {
            connection_->response_completed( *this );
        }
    }
}
