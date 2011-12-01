// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "bayeux/configuration.h"

namespace bayeux
{
	configuration::configuration()
		: max_disconnected_time_( boost::posix_time::seconds( 60 ) )
		, max_messages_per_client_( 10u )
		, max_messages_size_per_client_( 10 * 1024 )
	{
	}

	boost::posix_time::time_duration configuration::max_disconnected_time() const
	{
		return max_disconnected_time_;
	}

	configuration& configuration::max_disconnected_time( const boost::posix_time::time_duration& time_out )
	{
		max_disconnected_time_ = time_out;

		return *this;
	}

	unsigned configuration::max_messages_per_client() const
	{
		return max_messages_per_client_;
	}

	configuration& configuration::max_messages_per_client( unsigned new_limit )
	{
		max_messages_per_client_ = new_limit;

		return *this;
	}

	std::size_t configuration::max_messages_size_per_client() const
	{
		return max_messages_size_per_client_;
	}

	configuration& configuration::max_messages_size_per_client( std::size_t new_limit )
	{
		max_messages_size_per_client_ = new_limit;

		return *this;
	}

}
