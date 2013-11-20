#include "pubsub_http/response.h"
#include "tools/iterators.h"
#include <algorithm>

namespace pubsub
{
namespace http
{

namespace internal {

    const json::string id_token( "id" );
    const json::string cmd_token( "cmd" );
    const json::string response_token( "resp" );
    const json::string error_token( "error" );
    const json::string key_token( "key" );
    const json::string update_token( "update" );
    const json::string from_token( "from" );
    const json::string data_token( "data" );
    const json::string version_token( "version" );

    const json::string subscribe_token( "subscribe" );
    const json::string unsubscribe_token( "unsubscribe" );

}

using namespace internal;

static const json::string valid_message_tokens[] = { internal::id_token, internal::cmd_token };
static const json::string know_command_tokens[] = { internal::subscribe_token, internal::unsubscribe_token };

static bool known_command( const json::object& cmd )
{
    const std::vector< json::string > keys = cmd.keys();

    for ( std::vector< json::string >::const_iterator key = keys.begin(); key != keys.end(); ++key )
    {
        const json::string* const pos = std::find( tools::begin( know_command_tokens ), tools::end( know_command_tokens ), *key );

        if ( pos != tools::end( know_command_tokens ) )
            return true;
    }

    return false;
}

static bool check_cmd( const json::value& raw_cmd )
{
    const std::pair< bool, json::object > cmd = raw_cmd.try_cast< json::object >();
    return cmd.first && known_command( cmd.second );
}

static bool check_cmd_not_empty_and_valid( const json::value* const cmd_field )
{
    if ( !cmd_field )
        return false;

    const std::pair< bool, json::array > cmd_list = cmd_field->try_cast< json::array >();

    bool result = false;

    if ( cmd_list.first && !cmd_list.second.empty() )
    {
        result = true;
        for ( std::size_t cmd = 0; result && cmd != cmd_list.second.length(); ++cmd )
            result = check_cmd( cmd_list.second.at( cmd ) );
    }

    return result;
}

bool response_base::check_session_or_commands_given( const json::object& message, json::string& session_id ) const
{
    const std::vector< json::string > keys = message.keys();

    for ( std::vector< json::string >::const_iterator key = keys.begin(); key != keys.end(); ++key )
    {
        if ( std::find( tools::begin( valid_message_tokens ), tools::end( valid_message_tokens ), *key ) == tools::end( valid_message_tokens ) )
            return false;
    }

    const json::value* const session_id_field = message.find( internal::id_token );
    const json::value* const cmd_field        = message.find( internal::cmd_token );

    bool valid_session_given = false;

    if ( session_id_field )
    {
        std::pair< bool, json::string > id_as_string = session_id_field->try_cast< json::string >();

        if ( id_as_string.first && !id_as_string.second.empty() )
        {
            session_id          = id_as_string.second;
            valid_session_given = true;
        }
    }

    return valid_session_given || check_cmd_not_empty_and_valid( cmd_field );
}

json::object response_base::build_response( const json::string& session_id, const json::array& response, const json::array& updates ) const
{
    json::object result;
    result.add( id_token, session_id );

    if ( !response.empty() )
        result.add( response_token, response );

    if ( !updates.empty() )
        result.add( update_token, updates );

    return result;
}

bool response_base::check_node_name( const json::string cmd, const json::value& cmd_key_value, json::object& node_name, json::object& response )
{
    response.add( cmd, cmd_key_value );

    std::pair< bool, json::object > name = cmd_key_value.try_cast< json::object >();

    if ( !name.first )
        return ( response.add( error_token, json::string( "node name must be an object" ) ), false );

    if ( name.second.empty() )
        return ( response.add( error_token, json::string( "node name must not be empty" ) ), false );

    node_name = name.second;

    return true;
}

const char* response_base::name() const
{
    return "pubsub::http::response";
}

}
}





