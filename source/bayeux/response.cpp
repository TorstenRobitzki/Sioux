// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "bayeux/response.h"

#include "bayeux/bayeux.h"
#include "bayeux/node_channel.h"
#include "bayeux/session.h"
#include "pubsub/node.h"
#include "server/test_timer.h"

namespace bayeux
{

    template < class Timer >
	response_base< Timer >::response_base( connector< Timer >& con )
	    : session_( 0 )
		, connector_( con )
	{
	}

    template < class Timer >
	response_base< Timer >::~response_base()
	{
	    if ( session_ )
	        connector_.idle_session( session_ );
	}

    static const json::string id_token( "id" );
    static const json::string client_id_token( "clientId" );
	static const json::string channel_token( "channel" );
	static const json::string subscription_token( "subscription" );
	static const json::string connection_typen_token( "connectionType" );

	static const json::string meta_handshake_channel( "/meta/handshake" );
	static const json::string meta_connect_channel( "/meta/connect" );
	static const json::string meta_disconnect_channel( "/meta/disconnect" );
	static const json::string meta_subscribe_channel( "/meta/subscribe" );
    static const json::string meta_unsubscribe_channel( "/meta/unsubscribe" );

    template < class Timer >
	json::string response_base< Timer >::extract_channel( const json::object& request )
	{
		return request.at( channel_token ).upcast< json::string >();
	}

    template < class Timer >
	json::string response_base< Timer >::extract_client_id( const json::object& request )
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

    template < class Timer >
	pubsub::node_name response_base< Timer >::extract_node_name( const json::object& request )
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

    template < class Timer >
	json::object response_base< Timer >::add_session_id( const json::object& response, const json::string& session_id )
	{
		return response.copy().add( client_id_token, session_id );
	}

    template < class Timer >
	json::object response_base< Timer >::add_session_id( const json::object& response, const session& session_id )
	{
		return add_session_id( response, session_id.session_id() );
	}

    template < class Timer >
	void response_base< Timer >::handle_request( const json::object& request,
	    const boost::shared_ptr< response_interface >& self, const std::string& connection_name, bool last_message )
	{
		const json::string channel = extract_channel( request );

		if ( channel == meta_handshake_channel )
		{
			handle_handshake( request, connection_name );
			return;
		}

		const json::string client_id = check_client_id( request, channel );
		if ( client_id.empty() )
		    return;

		if ( !check_session( request, client_id, channel ) )
		    return;

        if ( channel == meta_connect_channel )
        {
            handle_connect( request, self, last_message );
        }
        else if ( channel == meta_disconnect_channel )
        {
            handle_disconnect( request );
        }
        else if ( channel == meta_subscribe_channel )
        {
            handle_subscribe( request );
        }
        else if ( channel == meta_unsubscribe_channel )
        {
            handle_unsubscribe( request );
        }
        else
        {
            throw std::runtime_error( "invalid channel: " + channel.to_std_string() );
        }
	}

    template < class Timer >
	void response_base< Timer >::handle_handshake( const json::object& request, const std::string& connection_name )
	{
		static const json::object response_prototype = json::parse_single_quoted(
			"{"
			"	'channel':'/meta/handshake',"
			"	'version':'1.0',"
			"	'supportedConnectionTypes':['long-polling'],"
			"	'successful':true"
			"}" ).upcast< json::object >();

		session_ = connector_.create_session( connection_name );
		json::object response = add_session_id( response_prototype, *session_ );
		copy_id_field( request, response );

		bayeux_response_.add( response );
	}


    template < class Timer >
	void response_base< Timer >::handle_connect( const json::object& request,
	    const boost::shared_ptr< response_interface >& self, bool last_message )
	{
	    assert( session_ );

	    if ( !check_connection_type( request, session_->session_id() ) )
	        return;

	    // when there are already messages to be send, then there is no point in blocking
	    const bool do_not_block = !bayeux_response_.empty() || !last_message;
	    const json::array messages = do_not_block ? session_->events() : session_->wait_for_events( self );

	    if ( !messages.empty() )
		{
	        bayeux_response_ += messages;
			bayeux_response_.add( build_connect_response( request, session_->session_id() ) );
		}
		else
		{
		    if ( do_not_block )
		    {
                bayeux_response_.add( build_connect_response( request, session_->session_id() ) );
		    }
		    else
		    {
                blocking_connect_ = request;
		    }
		}
	}

    template < class Timer >
    void response_base< Timer >::handle_disconnect( const json::object& request )
    {
        static const json::object response_prototype = json::parse_single_quoted(
            "{"
            "   'channel':'/meta/disconnect',"
            "   'successful':true"
            "}" ).upcast< json::object >();

        json::object response = add_session_id( response_prototype, *session_ );
        copy_id_field( request, response );

        bayeux_response_.add( response );
    }

    template < class Timer >
    json::object response_base< Timer >::build_connect_response( const json::object& request, const json::string& session_id )
    {
        static const json::object reponse_prototype = json::parse_single_quoted(
            "{"
            "   'channel'    : '/meta/connect',"
            "   'successful' : true"
            "}" ).upcast< json::object >();

        json::object response = add_session_id( reponse_prototype, session_id );
        copy_id_field( request, response );

        return response;
    }


    template < class Timer >
	void response_base< Timer >::handle_subscribe( const json::object& request )
	{
        assert( session_ );

        const json::string subscription = check_subscription( request, meta_subscribe_channel );

        session_->subscribe( node_name_from_channel( subscription ), request.find( id_token ) );
        bayeux_response_ += session_->events();
	}

    template < class Timer >
    void response_base< Timer >::handle_unsubscribe( const json::object& request )
    {
        assert( session_ );

        const json::string subscription = check_subscription( request, meta_subscribe_channel );

        session_->unsubscribe( node_name_from_channel( subscription ), request.find( id_token ) );
        bayeux_response_ += session_->events();
    }

    template < class Timer >
    json::string response_base< Timer >::check_client_id( const json::object& request, const json::string& response_channel )
    {
        const json::string id = extract_client_id( request );

        if ( id.empty() )
        {
            static const json::object response_template = json::parse_single_quoted(
                "{"
                "   'successful' : false,"
                "   'error'      : 'invalid clientId'"
                "}" ).upcast< json::object >();

            json::object response = response_template.copy();
            response.add( channel_token, response_channel );
            copy_id_field( request, response );

            bayeux_response_.add( response );
        }

        return id;
    }

    template < class Timer >
    bool response_base< Timer >::check_session( const json::object& request, const json::string& id,
        const json::string& response_channel )
    {
        if ( session_ )
        {
            connector_.idle_session( session_ );
            session_ = 0;
        }

        session_ = connector_.find_session( id );

        if ( session_ == 0 )
        {
            static const json::object response_template = json::parse_single_quoted(
                "{"
                "   'successful' : false,"
                "   'error'      : 'invalid clientId'"
                "}" ).upcast< json::object >();

            json::object response = response_template.copy();
            response.add( channel_token, response_channel );
            response.add( client_id_token, id );
            copy_id_field( request, response );

            bayeux_response_.add( response );
        }

        return session_ != 0;
    }

    template < class Timer >
    json::string response_base< Timer >::check_subscription( const json::object& request, const json::string& response_channel )
    {
        const json::value* const subscription = request.find( subscription_token );

        if ( subscription )
        {
            std::pair< bool, json::string > subscription_str = subscription->try_cast< json::string >();

            if ( subscription_str.first && !subscription_str.second.empty() )
                return subscription_str.second;
        }

        static const json::object response_template = json::parse_single_quoted(
            "{"
            "   'successful' : false,"
            "   'error'      : 'invalid clientId'"
            "}" ).upcast< json::object >();

        json::object response = response_template.copy();
        response.add( channel_token, response_channel );

        return json::string();
    }

    template < class Timer >
    bool response_base< Timer >::check_connection_type( const json::object& request, const json::string& session_id )
	{
	    const json::value* const type = request.find( connection_typen_token );

	    const bool result = type != 0 && *type == json::string( "long-polling" );

	    if ( !result )
	    {
	        static const json::object response_template = json::parse_single_quoted(
                "{"
                "   'channel'    : '/meta/connect',"
                "   'successful' : false,"
                "   'error'      : 'unsupported connection type'"
                "}" ).upcast< json::object >();

	        json::object response = add_session_id( response_template, session_id );
	        copy_id_field( request, response );

	        bayeux_response_.add( response );
	    }

	    return result;
	}

    template < class Timer >
    void response_base< Timer >::copy_id_field( const json::object& from, json::object& to ) const
    {
        const json::value* const id_value = from.find( id_token );

        if ( id_value )
            to.add( id_token, *id_value );
    }

    template < class Timer >
    std::vector< boost::asio::const_buffer > response_base< Timer >::build_response( const json::array& bayeux_response )
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


    template class response_base< boost::asio::deadline_timer >;
    template class response_base< server::test::timer >;
}
