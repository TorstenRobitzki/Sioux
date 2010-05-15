// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/response.h"

namespace server {

    async_response::async_response()
        : hurryed_(false)
    {
    }

    void async_response::hurry()
    {
        if ( !hurryed_ )
        {
            hurryed_ = true;
            implement_hurry();
        }
    }

    bool async_response::asked_to_hurry() const
    {
        return hurryed_;
    }

    void async_response::implement_hurry()
    {
    }

} // namespace server 


