#include "pubsub_http/response.h"
#include "pubsub_http/commands.h"
#include "tools/iterators.h"
#include <algorithm>
#include <map>

namespace pubsub
{
namespace http
{

namespace internal {

    const json::string id_token( "id" );
    const json::string cmd_token( "cmd" );
    const json::string response_token( "resp" );
    const json::string error_token( "error" );

    const json::string subscribe_token( "subscribe" );
    const json::string unsubscribe_token( "unsubscribe" );

}

using namespace ::pubsub::http::internal;

static const internal::subscribe    subscribe_cmd;
static const internal::unsubscribe  unsubscribe_cmd;

typedef std::map< json::string, const internal::command* > known_commands_list_t;

known_commands_list_t create_known_commands()
{
    known_commands_list_t result;
    result[ subscribe_token ] = &subscribe_cmd;
    result[ unsubscribe_token ] = &unsubscribe_cmd;

    return result;
}

static const known_commands_list_t know_commands = create_known_commands();
static const json::string valid_message_tokens[] = { id_token, cmd_token };

static const internal::command* find_command( const json::object& cmd )
{
    const std::vector< json::string > keys = cmd.keys();

    for ( std::vector< json::string >::const_iterator key = keys.begin(); key != keys.end(); ++key )
    {
        const known_commands_list_t::const_iterator pos = know_commands.find( *key );

        if ( pos != know_commands.end() )
            return pos->second;
    }

    return 0;
}

static bool check_cmd( const json::value& raw_cmd )
{
    const std::pair< bool, json::object > cmd = raw_cmd.try_cast< json::object >();

    return cmd.first && find_command( cmd.second );
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

bool response_base::check_session_or_commands_given( const json::object& message ) const
{
    const std::vector< json::string > keys = message.keys();

    for ( std::vector< json::string >::const_iterator key = keys.begin(); key != keys.end(); ++key )
    {
        if ( std::find( tools::begin( valid_message_tokens ), tools::end( valid_message_tokens ), *key ) == tools::end( valid_message_tokens ) )
            return false;
    }

    const json::value* const session_id = message.find( id_token );
    const json::value* const cmd_field  = message.find( cmd_token );

    return session_id || check_cmd_not_empty_and_valid( cmd_field );
}

json::value response_base::process_command( const json::value& command ) const
{
    json::object cmd = command.upcast< json::object >();

    const internal::command* const executer = find_command( cmd );
    assert( executer ); // the command was already looked up before

    return executer->execute( cmd );
}

json::array response_base::process_commands( const json::object& message ) const
{
    const json::value* const cmd_field = message.find( cmd_token );

    json::array result;

    if ( !cmd_field )
        return result;

    const json::array commands = cmd_field->upcast< json::array >();

    for ( std::size_t cmd_idx = 0, num_cmds = commands.length(); cmd_idx != num_cmds; ++cmd_idx )
    {
        const json::value response = process_command( commands.at( cmd_idx ) );

        if ( response != json::null() )
            result.add( response );
    }

    return result;
}

json::object response_base::build_response( const json::string& session_id, const json::array& response ) const
{
    json::object result;
    result.add( id_token, session_id );

    if ( !response.empty() )
        result.add( response_token, response );

    return result;
}

const char* response_base::name() const
{
    return "pubsub::http::response";
}

}
}





