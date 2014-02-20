
#include "server/response.h"
#include "http/http.h"
#include "json/json.h"

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/system/error_code.hpp>

namespace json
{
    /*
     * not template dependend implementation of response_base
     */
    class response_base : public server::async_response
    {
    public:
        const char* name() const;
    protected:
        explicit response_base( const json::value& );
        void build_response( http::http_error_code );

        const json::value                           output_;
        std::string                                 response_line_;
        std::string                                 body_size_;
        std::vector< boost::asio::const_buffer >    response_;
    };

    /**
     * @brief class, responsible to send a JSON encoded message response
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
        response( const boost::shared_ptr< Connection >& con, const json::value& val );

        /**
         * @brief responses with the given JSON value and the given http error code.
         */
        response( const boost::shared_ptr< Connection >& con, const json::value& val, http::http_error_code code );

        /**
         * @brief starts sending the response over the given connection.
         */
        void start();

    private:
        void response_written( const boost::system::error_code& ec, std::size_t size );

        const boost::shared_ptr< Connection >   connection_;
    };


    // implementation
    template < class Connection >
    response< Connection >::response( const boost::shared_ptr< Connection >& con, const json::value& val )
        : response_base( val )
        , connection_( con )
    {
        build_response( http::http_ok );
    }

    template < class Connection >
    response< Connection >::response( const boost::shared_ptr< Connection >& con, const json::value& val, http::http_error_code code )
        : response_base( val )
        , connection_( con )
    {
        build_response( code );
    }

    template < class Connection >
    void response< Connection >::start()
    {
        connection_->async_write(
            response_, boost::bind( &response::response_written, this->shared_from_this(), _1, _2 ), *this );
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
