// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "pubsub/key.h"

namespace pubsub {

    /////////////////////
    // class key_domain
    key_domain::key_domain(const std::string& name)
        : domain_name_(name)
    {
    }

    bool key_domain::operator<(const key_domain& rhs) const
    {
        return domain_name_ < rhs.domain_name_;
    }

} // namespace pubsub

