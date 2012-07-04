// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/secure_session_generator.h"
#include <sstream>

namespace server
{
    std::string secure_session_generator::operator()( const char* /* network_connection_name */ )
    {
        std::stringstream out;
        out << std::hex << distribution_( generator_ );

        return out.str();
    }

}
