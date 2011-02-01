// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "pubsub/key.h"
#include <ostream>

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

    std::ostream& operator<<(std::ostream& out, const key_domain& k)
    {
        return out << k.name();
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

    void key::print(std::ostream& out) const
    {
        out << domain_ << ":" << value_;
    }

    std::ostream& operator<<(std::ostream& out, const key& k)
    {
        k.print(out);
        return out;
    }

} // namespace pubsub

