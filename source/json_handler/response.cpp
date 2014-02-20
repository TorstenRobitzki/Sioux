#include "json_handler/response.h"
#include "http/server_header.h"
#include "tools/asstring.h"

namespace json {

    response_base::response_base( const json::value& val )
        : output_( val )
    {
    }

    const char* response_base::name() const
    {
        return "json::response";
    }

    void response_base::build_response( http::http_error_code code )
    {
        static const char response_header[] =
            "Content-Type: application/json\r\n"
            SIOUX_SERVER_HEADER
            "Content-Length: ";

        response_line_ = http::status_line( "1.1", code );
        body_size_     = tools::as_string( output_.size() ) + "\r\n\r\n";

        response_.push_back( boost::asio::buffer( response_line_ ) );
        response_.push_back( boost::asio::buffer( response_header, sizeof response_header -1 ) );
        response_.push_back( boost::asio::buffer( body_size_ ) );
        output_.to_json( response_ );
    }

}
