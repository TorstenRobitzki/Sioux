// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "bayeux/response.h"

#include "bayeux/bayeux.h"
#include "bayeux/node_channel.h"
#include "bayeux/session.h"
#include "pubsub/node.h"

namespace bayeux
{

	response_base::response_base( connector& con )
		: connector_( con )
	{
	}

    void response_base::second_connection_detected()
    {
        // and now?
    }

    void response_base::new_messages( const json::array& )
    {

    }

    static const json::string client_id_token( "clientId" );
	static const json::string channel_token( "channel" );
	static const json::string subscription_token( "subscription" );

	std::string response_base::extract_channel( const json::object& request )
	{
		return request.at( channel_token ).upcast< json::string >().to_std_string();
	}

	json::string response_base::extract_client_id( const json::object& request )
	{
        const json::value* const id = request.find( client_id_token );

        struct : json::default_visitor
        {
            void visit( const json::string& v)
            {
            	value = v;
            }
        	json::string value;
        } visitor;

        if ( id )
        	id->visit( visitor );

        return visitor.value;
	}

	pubsub::node_name response_base::extract_node_name( const json::object& request )
	{
        const json::value* const channel = request.find( subscription_token );

        struct : json::default_visitor
        {
            void visit( const json::string& v)
            {
            	value = node_name_from_channel( v );
            }

            pubsub::node_name value;
        } visitor;

        if ( channel )
        	channel->visit( visitor );

        return visitor.value;
	}

	json::object response_base::add_session_id( const json::object& response, const json::string& session_id )
	{
		return response.copy().add( client_id_token, session_id );
	}

	json::object response_base::add_session_id( const json::object& response, const session& session_id )
	{
		return add_session_id( response, session_id.session_id() );
	}


	void response_base::handle_request( const json::object& request, const std::string& connection_name )
	{
		const std::string channel = extract_channel( request );

		if ( channel == "/meta/handshake" )
		{
			handle_handshake( request, connection_name );
		}
		else if ( channel == "/meta/connect" )
		{
			handle_connect( request, connection_name );
		}
		else if ( channel == "/meta/subscribe")
		{
			handle_subscribe( request, connection_name );
		}
		else
		{
			throw "foo";
		}
	}

	void response_base::handle_handshake( const json::object& request, const std::string& connection_name )
	{
		static const json::object response_prototype = json::parse_single_quoted(
			"{"
			"	'channel':'/meta/handshake',"
			"	'version':'1.0',"
			"	'supportedConnectionTypes':['long-polling'],"
			"	'successful':true"
			"}" ).upcast< json::object >();

		const boost::shared_ptr< const session > new_session = connector_.create_session( connection_name );
		bayeux_response_.add( add_session_id( response_prototype, *new_session ) );
	}

	void response_base::handle_connect( const json::object& request, const std::string& connection_name )
	{
		const json::string id = extract_client_id( request );

		const boost::shared_ptr< session > existing_session = id.empty()
				? boost::shared_ptr< session >()
				: connector_.find_session( id );

		if ( existing_session.get() )
		{
			static const json::object reponse_prototype = json::parse_single_quoted(
				"{"
				"	'channel'    : '/meta/connect',"
				"	'successful' : true"
				"}" ).upcast< json::object >();

			bayeux_response_.add( add_session_id( reponse_prototype, *existing_session ) );
		}
		else
		{
			static const json::object failed_response_prototype = json::parse_single_quoted(
				"{"
				"	'channel'    : '/meta/connect',"
				"	'successful' : false,"
				"	'error'		 : 'invalid clientId'"
				"}" ).upcast< json::object >();

			bayeux_response_.add( add_session_id( failed_response_prototype, id ) );
		}

	}

	void response_base::handle_subscribe( const json::object& request, const std::string& connection_name )
	{
		const json::string 			id = extract_client_id( request );
		const pubsub::node_name 	node_name = extract_node_name( request );

		if ( !id.empty() && node_name != pubsub::node_name() )
		{
			static const json::object response_prototype = json::parse_single_quoted(
				"{"
				"	'channel':'/meta/subscribe',"
				"	'"
				"	'version':'1.0',"
				"	'supportedConnectionTypes':['long-polling'],"
				"	'successful':true"
				"}" ).upcast< json::object >();

			const boost::shared_ptr< const session > new_session = connector_.create_session( connection_name );
			bayeux_response_.add( add_session_id( response_prototype, *new_session ) );
		}
		else
		{
			static const json::object failed_response_prototype = json::parse_single_quoted(
				"{"
				"	'channel'    : '/meta/subscribe',"
				"	'successful' : false,"
				"	'error'		 : 'invalid subscription'"
				"}" ).upcast< json::object >();

			bayeux_response_.add( add_session_id( failed_response_prototype, id ) );
		}
	}

	std::vector< boost::asio::const_buffer > response_base::build_response( const json::array& bayeux_response )
	{
		static const char response_header[] =
			"HTTP/1.1 200 OK\r\n"
			"Content-Type : application/json\r\n"
			"Content-Length : ";

		response_buffer_ = tools::as_string( bayeux_response.size() ) + "\r\n\r\n";

		std::vector< boost::asio::const_buffer > result;

		result.push_back( boost::asio::buffer( response_header, sizeof response_header -1 ) );
		result.push_back( boost::asio::buffer( response_buffer_ ) );
		bayeux_response.to_json( result );

		return result;
	}

}
