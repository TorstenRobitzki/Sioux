// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "pubsub/node.h"
#include "tools/asstring.h"
#include <algorithm>

namespace pubsub {

    namespace 
    {
        struct compare_domains : std::binary_function<std::string, std::string, bool>
        {
            bool operator()(const std::string& lhs, const std::string& rhs) const
            {
                return key_domain(lhs) < key_domain(rhs);
            }
        };

        struct to_domain
        {
            key_domain operator()(const json::string& s) const
            {
                return key_domain(tools::as_string(s));
            }
        };
    }

    ////////////////////
    // class node_name
    node_name::node_name()
    {
    }

    node_name::node_name(const json::object& keys)
    {
        const std::vector<json::string> json_key_names = keys.keys();
        std::vector<key_domain>         domains;
        
        std::transform(json_key_names.begin(), json_key_names.end(), std::back_inserter(domains), to_domain());
        std::sort(domains.begin(), domains.end());

        for ( std::vector<key_domain>::const_iterator k = domains.begin(); k != domains.end(); ++k )
        {
            keys_.push_back(key(*k, tools::as_string(keys.at(json::string(k->name().c_str())))));
        }
    }

    bool node_name::operator==(const node_name& rhs) const
    {
        if ( keys_.size() != rhs.keys_.size() )
            return false;

        key_list::const_iterator lhs_begin = keys_.begin();
        key_list::const_iterator rhs_begin = rhs.keys_.begin();

        for ( ; lhs_begin != keys_.end() && *lhs_begin == *rhs_begin; ++lhs_begin, ++rhs_begin )
            ;

        return lhs_begin == keys_.end();
    }

    bool node_name::operator<(const node_name& rhs) const
    {
        if ( keys_.size() != rhs.keys_.size() )
            return keys_.size() < rhs.keys_.size();

        key_list::const_iterator lhs_begin = keys_.begin();
        key_list::const_iterator rhs_begin = rhs.keys_.begin();

        for ( ; lhs_begin != keys_.end(); ++lhs_begin, ++rhs_begin )
        {
            if ( *lhs_begin < *rhs_begin )
                return true;

            if ( *rhs_begin < *lhs_begin )
                return false;
        }

        return false;
    }

    namespace {
        struct sort_by_domain : std::binary_function<key, key, bool>
        {
            bool operator()(const key& lhs, const key& rhs) const
            {
                return lhs.domain() < rhs.domain();
            }
        };
    }

    std::pair<bool, key> node_name::find_key(const key_domain& domain) const
    {
        std::pair<bool, key> result(false, key());

        key_list::const_iterator pos = std::lower_bound(keys_.begin(), keys_.end(), key(domain, std::string()), sort_by_domain());

        if ( pos != keys_.end() && !(pos->domain() < domain) )
            result = std::make_pair(true, *pos);

        return result;
    }

    ///////////////
    // class node
    node::node(const node_version& first_version, const json::value& first_versions_data)
        : versions_(1u, first_versions_data)
        , version_(first_version)
    {
    }

    node_version node::current_version() const
    {
        return version_;
    }

    node_version node::oldest_version() const
    {
        return version_ - versions_.size();
    }

    const json::value& node::data() const
    {
        return versions_.front();
    }

    std::pair<bool, json::value> node::update_from(const node_version& /* known_version */) const
    {
        return std::make_pair(false, json::string());
    }

    bool node::operator==(const node& rhs) const
    {
        return versions_ == rhs.versions_ && version_ == rhs.version_;
    }

} // namespace pubsub

