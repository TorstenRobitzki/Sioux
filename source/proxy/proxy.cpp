// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "proxy/proxy.h"


namespace proxy
{
    error::error(const std::string& e) :  std::runtime_error(e)
    {
    }
} // namespace server 


