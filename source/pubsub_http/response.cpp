#include "pubsub_http/response.h"
#include "tools/iterators.h"
#include <algorithm>

namespace pubsub
{
namespace http
{

static const json::string id_token( "id" );
static const json::string cmd_token( "cmd" );

static const json::string valid_message_tokens[] = { id_token, cmd_token };

bool response_base::check_protocol_message( const json::object& message ) const
{
    const std::vector< json::string > keys = message.keys();

    for ( std::vector< json::string >::const_iterator key = keys.begin(); key != keys.end(); ++key )
    {
        if ( std::find( tools::begin( valid_message_tokens ), tools::end( valid_message_tokens ), *key ) == tools::end( valid_message_tokens ) )
            return false;
    }

    return true;
}

const char* response_base::name() const
{
    return "pubsub::http::response";
}

}
}





