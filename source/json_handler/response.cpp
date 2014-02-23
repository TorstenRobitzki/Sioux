#include "json_handler/response.h"
#include "http/server_header.h"
#include "tools/asstring.h"

namespace json {

    response_base::response_base( const boost::shared_ptr< const http::request_header >& request, const handler_t& handler )
        : request_( request )
        , handler_( handler )
        , response_body_( json::null() ) // dummy default
    {
        assert( request.get() );
        assert( !handler_.empty() );
    }

    const char* response_base::name() const
    {
        return "json::response";
    }

    void response_base::build_response( const json::value& response_body )
    {
        static const char response_header[] =
            "Content-Type: application/json\r\n"
            SIOUX_SERVER_HEADER
            "Content-Length: ";

        const std::pair< json::value, http::http_error_code > handler_result = handler_( *request_, response_body );

        response_body_ = handler_result.first;
        response_line_ = http::status_line( "1.1", handler_result.second );
        body_size_     = tools::as_string( response_body_.size() ) + "\r\n\r\n";

        response_.push_back( boost::asio::buffer( response_line_ ) );
        response_.push_back( boost::asio::buffer( response_header, sizeof response_header -1 ) );
        response_.push_back( boost::asio::buffer( body_size_ ) );
        response_body_.to_json( response_ );
    }

}
