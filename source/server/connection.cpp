// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/connection.h"

namespace server {

    const char* connection_error_category::name() const
    {
        return "server.connection";
    }

    std::string connection_error_category::message( int  ) const
    {
        return "server.connection error";
    }

    boost::system::error_code make_error_code(error_codes e)
    {
        static connection_error_category cat;
        return boost::system::error_code(e, &cat);
    }


} // namespace server

