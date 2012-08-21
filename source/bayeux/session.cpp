// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "bayeux/session.h"

#include "bayeux/configuration.h"
#include "bayeux/node_channel.h"
#include "bayeux/response.h"
#include "pubsub/root.h"

namespace bayeux
{
	static const json::string channel_tag( "channel" );
	static const json::string subscription_tag( "subscription" );
	static const json::string client_id_tag( "clientId" );
	static const json::string data_tag( "data" );
	static const json::string id_tag( "id" );
	static const json::string subscription_error_tag( "#error" );
    static const json::string error_tag( "error" );

	session::session( const std::string& session_id, pubsub::root& data, const boost::shared_ptr< const configuration >& config )
		: session_id_( session_id.c_str() )
	    , root_( data )
		, mutex_()
	    , messages_()
		, http_connection_()
		, config_( config )
	    , subscription_mutex_()
	    , subscription_ids_()
	{
	}

	const json::string& session::session_id() const
	{
		return session_id_;
	}

	json::array session::wait_for_events( const boost::shared_ptr< response_interface >& response )
	{
	    assert( response.get() );

        json::array                              updates;
        boost::shared_ptr< response_interface >  old_connection;

        {
            boost::mutex::scoped_lock lock( mutex_ );

            if ( messages_.empty() )
            {
                old_connection.swap( http_connection_ );
                http_connection_ = response;
            }
            else
            {
                assert( http_connection_.get() == 0 );
                messages_.swap( updates );
            }
        }

        if ( old_connection.get() )
            old_connection->second_connection_detected();

        return updates;
	}

	json::array session::events()
	{
		json::array updates;

		{
			boost::mutex::scoped_lock lock( mutex_ );
			messages_.swap( updates );
		}

		return updates;
	}

    void session::subscribe( const pubsub::node_name& name, const json::value* id )
    {
        if ( name.empty() )
            throw std::runtime_error( "subscribing with empty node_name" );

        {
            boost::mutex::scoped_lock lock( subscription_mutex_ );

            subscription_context  context = { id ? *id : json::null(), id != 0 };
            subscription_ids_.insert( std::make_pair( name, context ) );
        }

        root_.subscribe( shared_from_this(), name );
    }

    void session::unsubscribe( const pubsub::node_name& node, const json::value* id )
    {
        json::value subscribe_response = json::null();

        /* check if there is still a subscription response is outstanding */
        {
            boost::mutex::scoped_lock lock( subscription_mutex_ );

            subscription_ids_t::iterator pos = subscription_ids_.find( node );

            if ( pos != subscription_ids_.end() )
            {
                subscribe_response = build_subscription_success_msg( pos->second, node );
                subscription_ids_.erase( pos );
            }
        }

        const json::object unsubscribe_response = root_.unsubscribe( shared_from_this(), node )
                ? build_unsubscribe_success_msg( node, id )
                : build_unsubscribe_error_msg( node, id );

        if ( subscribe_response == json::null() )
        {
            add_message_and_notify( unsubscribe_response );
        }
        else
        {
            json::array responses;
            responses.add( subscribe_response );
            responses.add( unsubscribe_response );

            add_messages_and_notify( responses );
        }
    }

    void session::timeout()
    {
        boost::shared_ptr< response_interface >  old_connection;

        {
            boost::mutex::scoped_lock lock( mutex_ );

            if ( http_connection_.get() )
            {
                assert( messages_.empty() );
                http_connection_.swap( old_connection );
            }
        }

        if ( old_connection.get() )
            old_connection->messages( json::array(), session_id_ );
    }

    void session::hurry()
    {
        timeout();
    }

    void session::close()
    {
        root_.unsubscribe_all( shared_from_this() );

        boost::mutex::scoped_lock lock( mutex_ );
        http_connection_.reset();
    }

    void session::shut_down()
    {
        json::array                              updates;
        boost::shared_ptr< response_interface >  old_connection;
        root_.unsubscribe_all( shared_from_this() );

        {
            boost::mutex::scoped_lock lock( mutex_ );

            if ( http_connection_.get() )
            {
                http_connection_.swap( old_connection );
                messages_.swap( updates );
            }
        }

        if ( old_connection.get() )
            old_connection->messages( updates, session_id_ );
    }


    boost::posix_time::time_duration session::long_polling_timeout() const
    {
        boost::mutex::scoped_lock lock( mutex_ );

        return config_->long_polling_timeout();
    }

    void session::on_update(const pubsub::node_name& name, const pubsub::node& data)
    {
        // there is no different in receiving the initial data after a subscription,
	    // or updated data. If there is an entry for the subject, the subscription wasn't
	    // acknowledged yet
        json::array response_list;

	    {
	        boost::mutex::scoped_lock lock( subscription_mutex_ );
	        subscription_ids_t::iterator pos = subscription_ids_.find( name );

	        if ( pos != subscription_ids_.end() )
	        {
	            response_list.add( build_subscription_success_msg( pos->second, name ) );
	            subscription_ids_.erase( pos );
	        }
	    }

	    const json::object update_msg = build_update_msg( name, data );

	    if ( response_list.empty() && !update_msg.empty() )
	    {
	        add_message_and_notify( update_msg );
	    }
	    else if ( !response_list.empty() && !update_msg.empty() )
	    {
            response_list.add( update_msg );
	        add_messages_and_notify( response_list );
	    }
	    else if ( !response_list.empty() )
	    {
	        add_message_and_notify( response_list.at( 0 ).upcast< json::object >() );
	    }
    }

    void session::on_invalid_node_subscription(const pubsub::node_name& node)
    {
        static const json::string error_msg( "invalid subscription" );
        add_message_and_notify( build_subscription_error_msg( error_msg, node ) );
    }

    void session::on_unauthorized_node_subscription(const pubsub::node_name& node)
    {
        static const json::string error_msg( "authorization failed" );
        add_message_and_notify( build_subscription_error_msg( error_msg, node ) );
    }

    void session::on_failed_node_subscription(const pubsub::node_name& node)
    {
        static const json::string error_msg( "initialization failed" );
        add_message_and_notify( build_subscription_error_msg( error_msg, node ) );
    }

    json::object session::build_subscription_error_msg( const json::string& error_msg, const pubsub::node_name& node )
    {
        static const json::object message_prototype = json::parse_single_quoted(
            "{"
            "   'channel'    : '/meta/subscribe',"
            "   'successful' : false"
            "}" ).upcast< json::object >();

        json::object message = message_prototype.copy();
        message.add( error_tag, error_msg );
        message.add( subscription_tag, channel_from_node_name( node ) );
        message.add( client_id_tag, session_id_ );
        add_subscription_id_if_exists( node, message );

        return message;
    }

    json::object session::build_subscription_success_msg( const subscription_context& context, const pubsub::node_name& node ) const
    {
        static const json::object message_prototype = json::parse_single_quoted(
            "{"
            "   'channel'    : '/meta/subscribe',"
            "   'successful' : true"
            "}" ).upcast< json::object >();

        json::object message = message_prototype.copy();
        message.add( subscription_tag, channel_from_node_name( node ) );
        message.add( client_id_tag, session_id_ );

        if ( context.id_valid_ )
            message.add( id_tag, context.id_ );

        return message;
    }

    json::object session::build_update_msg( const pubsub::node_name& name, const pubsub::node& data ) const
    {
        struct object_visitor : json::default_visitor
        {
            void visit( const json::object& o )
            {
                const json::value* const data = o.find( data_tag );

                if ( data )
                {
                    message.add( data_tag, *data );
                    const json::value* id = o.find( id_tag );

                    if ( id )
                        message.add( id_tag, *id );

                    message.add( channel_tag, channel_from_node_name( name ) );
                }
            }

            json::object             message;
            const pubsub::node_name& name;

            object_visitor( const pubsub::node_name& n ) : name( n ) {}
        } visitor( name );

        data.data().visit( visitor );

        return visitor.message;
    }

    json::object session::build_unsubscribe_error_msg( const pubsub::node_name& node, const json::value* id ) const
    {
        static const json::object message_prototype = json::parse_single_quoted(
            "{"
            "   'channel'    : '/meta/unsubscribe',"
            "   'successful' : false,"
            "   'error'      : 'not subscribed'"
            "}" ).upcast< json::object >();

        json::object message = message_prototype.copy();
        message.add( subscription_tag, channel_from_node_name( node ) );
        message.add( client_id_tag, session_id_ );

        if ( id )
            message.add( id_tag, *id );

        return message;
    }

    json::object session::build_unsubscribe_success_msg( const pubsub::node_name& node, const json::value* id ) const
    {
        static const json::object message_prototype = json::parse_single_quoted(
            "{"
            "   'channel'    : '/meta/unsubscribe',"
            "   'successful' : true"
            "}" ).upcast< json::object >();

        json::object message = message_prototype.copy();
        message.add( subscription_tag, channel_from_node_name( node ) );
        message.add( client_id_tag, session_id_ );

        if ( id )
            message.add( id_tag, *id );

        return message;
    }

    void session::add_subscription_id_if_exists( const pubsub::node_name& name, json::object& message )
    {
        boost::mutex::scoped_lock lock( subscription_mutex_ );

        subscription_ids_t::iterator pos = subscription_ids_.find( name );
        assert( pos != subscription_ids_.end() );

        if ( pos->second.id_valid_ )
            message.add( id_tag, pos->second.id_ );

        subscription_ids_.erase( pos );
    }

    void session::add_message_impl( const json::value& new_message )
    {
        messages_.add( new_message );

        while ( messages_.length() > config_->max_messages_per_client()
            || messages_.size() > config_->max_messages_size_per_client() )
            messages_.erase( 0, 1u );
    }

    void session::add_message_and_notify( const json::object& new_message )
    {
        json::array                              updates;
        boost::shared_ptr< response_interface >  old_connection;

        {
            boost::mutex::scoped_lock lock( mutex_ );

            add_message_impl( new_message );

            if ( http_connection_.get() )
            {
                http_connection_.swap( old_connection );
                messages_.swap( updates );
            }
        }

        if ( old_connection.get() )
            old_connection->messages( updates, session_id_ );
    }

    void session::add_messages_and_notify( const json::array& new_messages )
    {
        json::array                              updates;
        boost::shared_ptr< response_interface >  old_connection;

        {
            boost::mutex::scoped_lock lock( mutex_ );

            for ( std::size_t i = 0; i != new_messages.length(); ++i )
                add_message_impl( new_messages.at( i ) );

            if ( http_connection_.get() )
            {
                http_connection_.swap( old_connection );
                messages_.swap( updates );
            }
        }

        if ( old_connection.get() )
            old_connection->messages( updates, session_id_ );
    }


} // namespace bayeux

