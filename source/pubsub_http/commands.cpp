#include "pubsub_http/commands.h"
#include "pubsub_http/response.h"
#include "pubsub_http/sessions.h"
#include "pubsub/node.h"
#include "pubsub/root.h"
#include "json/json.h"

namespace pubsub {
namespace http {
namespace internal {
#if 0
    static bool check_node_name( const json::object& command, const json::string& cmd_token, json::object& node_name, json::object& response )
    {
        const json::value cmd_key_value = command.at( cmd_token );
        response.add( cmd_token, cmd_key_value );

        std::pair< bool, json::object > name = cmd_key_value.try_cast< json::object >();

        if ( !name.first )
            return ( response.add( error_token, json::string( "node name must be an object" ) ), false );

        if ( name.second.empty() )
            return ( response.add( error_token, json::string( "node name must not be empty" ) ), false );

        node_name = name.second;

        return true;
    }

    json::value subscribe::execute( const json::object& command, pubsub::root& root, session_reference& subscriber ) const
    {
        json::object response;
        json::object node;

        if ( !check_node_name( command, subscribe_token, node, response ) )
            return response;

        const pubsub::node_name name( node );
        //   root.subscribe( subscriber, name );

        return json::null();
    }

    json::value unsubscribe::execute( const json::object& command, pubsub::root&, session_reference& ) const
    {
        json::object response;
        json::object node_name;

        if ( !check_node_name( command, unsubscribe_token, node_name, response ) )
            return response;

        return json::null();
    }
#endif
}
}
}
