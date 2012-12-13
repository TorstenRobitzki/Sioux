// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "bayeux/configuration.h"
#include <ostream>

namespace bayeux
{
	configuration::configuration()
		: max_disconnected_time_( boost::posix_time::seconds( 60 ) )
	    , long_polling_timeout_( boost::posix_time::seconds( 20 ) )
		, max_messages_per_client_( 10u )
		, max_messages_size_per_client_( 10 * 1024 )
	    , reconnect_advice_( handshake )
	{
	}

	boost::posix_time::time_duration configuration::session_timeout() const
	{
		return max_disconnected_time_;
	}

	configuration& configuration::session_timeout( const boost::posix_time::time_duration& time_out )
	{
		max_disconnected_time_ = time_out;

		return *this;
	}

    boost::posix_time::time_duration configuration::long_polling_timeout() const
    {
        return long_polling_timeout_;
    }

    configuration& configuration::long_polling_timeout( const boost::posix_time::time_duration& time_out )
    {
        long_polling_timeout_ = time_out;

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

	configuration::reconnect_advice_t configuration::reconnect_advice() const
	{
	    return reconnect_advice_;
	}

	configuration& configuration::reconnect_advice( reconnect_advice_t new_advice )
	{
	    reconnect_advice_ = new_advice;

	    return *this;
	}

    std::ostream& operator<<( std::ostream& out, configuration::reconnect_advice_t advice )
    {
        switch ( advice )
        {
        case configuration::retry:
            return out << "retry";
        case configuration::handshake:
            return out << "handshake";
        case configuration::none:
            return out << "none";
        }

        return out << "configuration::reconnect_advice_t(" << int( advice ) << ")";
    }

}
