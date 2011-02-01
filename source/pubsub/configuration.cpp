// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "pubsub/configuration.h"

namespace pubsub
{
    configuration::configuration()
        : node_timeout_()
        , min_update_period_()
        , max_update_size_(70u)
    {
    }

    boost::posix_time::time_duration    configuration::node_timeout() const
    {
        return node_timeout_;
    }

    void configuration::node_timeout(const boost::posix_time::time_duration& new_timeout)
    {
        node_timeout_ = new_timeout;
    }

    boost::posix_time::time_duration    configuration::min_update_period() const
    {
        return min_update_period_;
    }

    unsigned configuration::max_update_size() const
    {
        return max_update_size_;
    }

    bool configuration::authorization_required() const
    {
        return authorization_required_;
    }

    void configuration::authorization_required(bool new_value)
    {
        authorization_required_ = new_value;
    }

    ////////////////////////
    // class configurator
    const configurator& configurator::node_timeout(const boost::posix_time::time_duration& d) const
    {
        config_.node_timeout(d);
        return *this;
    }

    const configurator& configurator::authorization_required() const
    {
        config_.authorization_required(true);
        return *this;
    }

    const configurator& configurator::authorization_not_required() const
    {
        config_.authorization_required(false);
        return *this;
    }

    configurator::operator const configuration() const
    {
        return config_;
    }


} // namespace pubsub


