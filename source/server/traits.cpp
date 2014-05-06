// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/traits.h"

namespace server 
{
    connection_config::connection_config()
        : timeout_( boost::posix_time::seconds( 3 ) )
    {

    }

    boost::posix_time::time_duration connection_config::keep_alive_timeout() const
    {
        return boost::posix_time::seconds( 30 );
    }

    boost::posix_time::time_duration connection_config::timeout() const
    {
        return timeout_;
    }

    void connection_config::timeout( const boost::posix_time::time_duration& new_timeout )
    {
        timeout_ = new_timeout;
    }

    boost::posix_time::time_duration connection_config::reaccept_timeout() const
    {
        return boost::posix_time::seconds( 1 );
    }

} // namespace server 

