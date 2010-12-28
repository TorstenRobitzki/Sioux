// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "pubsub/key.h"

namespace pubsub {

    /////////////////////
    // class key_domain
    key_domain::key_domain()
    {
    }

    key_domain::key_domain(const std::string& name)
        : domain_name_(name)
    {
    }

    bool key_domain::operator<(const key_domain& rhs) const
    {
        return domain_name_ < rhs.domain_name_;
    }

    bool key_domain::operator==(const key_domain& rhs) const
    {
        return domain_name_ == rhs.domain_name_;
    }

    bool key_domain::operator!=(const key_domain& rhs) const
    {
        return domain_name_ != rhs.domain_name_;
    }

    const std::string& key_domain::name() const
    {
        return domain_name_;
    }

    //////////////
    // class key
    key::key()
    {
    }

    key::key(const key_domain& d, const std::string& v)
        : domain_(d)
        , value_(v)
    {
    }

    bool key::operator==(const key& rhs) const
    {
        return domain_ == rhs.domain_ && value_ == rhs.value_;
    }

    bool key::operator<(const key& rhs) const
    {
        return domain_ < rhs.domain_
            || domain_ == rhs.domain_ && value_ < rhs.value_;
    }

    const key_domain& key::domain() const
    {
        return domain_;
    }

} // namespace pubsub

