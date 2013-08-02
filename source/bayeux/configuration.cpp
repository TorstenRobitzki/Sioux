// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "bayeux/configuration.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <ostream>

namespace bayeux
{
	configuration::configuration()
		: max_disconnected_time_( boost::posix_time::seconds( 60 ) )
	    , long_polling_timeout_( boost::posix_time::seconds( 20 ) )
		, max_messages_per_client_( 10u )
		, max_messages_size_per_client_( 10 * 1024 )
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

    void configuration::print( std::ostream& out ) const
    {
        out << "max_disconnected_time: " << max_disconnected_time_;
        out << "\nlong_polling_timeout: " << long_polling_timeout_;
        out << "\nmax_messages_per_client: " << max_messages_per_client_;
        out << "\nmax_messages_size_per_client: " << max_messages_size_per_client_ << "\n";
    }

    std::ostream& operator<<( std::ostream& out, const configuration& config )
    {
        config.print( out );
        return out;
    }

}
