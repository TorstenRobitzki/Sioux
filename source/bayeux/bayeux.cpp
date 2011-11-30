// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "bayeux/bayeux.h"

#include "bayeux/configuration.h"
#include "bayeux/session.h"

namespace bayeux
{
	connector::connector( pubsub::root& data, server::session_generator& session_generator, const configuration& config )
		: data_( data )
		, session_generator_( session_generator )
		, mutex_()
		, current_config_( new configuration( config ) )
		, sessions_()
	{
	}

	boost::shared_ptr< session > connector::find_session( const json::string& session_id )
	{
		boost::mutex::scoped_lock lock( mutex_ );

		const session_list_t::const_iterator pos = sessions_.find( session_id.to_std_string() );
		return pos == sessions_.end() ? boost::shared_ptr< session >() : pos->second;
	}

	boost::shared_ptr< session > connector::create_session( const std::string& network_connection_name )
	{
		boost::shared_ptr< session > result;

		{
			boost::mutex::scoped_lock lock( generator_mutex_ );
			result.reset( new session( session_generator_( network_connection_name ), data_, current_config_ ) );
		}

		boost::mutex::scoped_lock lock( mutex_ );
		sessions_[ result->session_id().to_std_string() ] = result;

		return result;
	}

}

