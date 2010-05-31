// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/proxy.h"


namespace server 
{


std::string proxy_config_base::modified_host(const std::string& original_host)
{
    return  original_host;
}

std::string proxy_config_base::modified_url(const std::string& original_url)
{
    return original_url;
}

} // namespace server 


