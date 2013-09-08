#include "asio_mocks/json_msg.h"
#include "tools/asstring.h"
#include "json/json.h"

namespace asio_mocks {

    static read json_msg_impl( const std::string& body )
    {
        const std::string result =
            "POST / HTTP/1.1\r\n"
            "Host: test-server.de\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: "
         + tools::as_string( body.size() )
         + "\r\n\r\n"
         +  body;

        return read( result.begin(), result.end() );
    }

    read json_msg( const std::string& txt )
    {
        std::string body( txt );
        std::replace( body.begin(), body.end(), '\'', '\"' );

        return json_msg_impl( body );
    }

    read json_msg( const json::value& payload )
    {
        return json_msg_impl( payload.to_json() );
    }
}
