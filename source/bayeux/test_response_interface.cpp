// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "bayeux/test_response_interface.h"
#include "tools/asstring.h"

namespace bayeux
{
namespace test
{
    response_interface::response_interface()
        : second_connections_detected_( 0 )
    {
    }

    int response_interface::number_of_second_connection_detected() const
    {
        return second_connections_detected_;
    }

    json::array response_interface::new_message() const
    {
        if ( messages_.size() != 1u )
            throw std::runtime_error( "no single message reported: " + tools::as_string( messages_.size() ) );

        return messages_.front();
    }

    const std::vector< json::array >& response_interface::messages() const
    {
        return messages_;
    }

    void response_interface::second_connection_detected()
    {
        ++second_connections_detected_;
    }

    void response_interface::messages( const json::array& message, const json::string& )
    {
        messages_.push_back( message );
    }

}
}
