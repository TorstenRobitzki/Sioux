// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/response.h"
#include "server/log.h"

#include <iostream>

namespace server
{
    stream_event_log::stream_event_log()
        : mutex_()
        , out_( std::cout )
        , connection_cnt_( 0 )
    {
    }

    stream_error_log::stream_error_log()
        : mutex_()
        , log_( std::cerr )
    {
    }
}
