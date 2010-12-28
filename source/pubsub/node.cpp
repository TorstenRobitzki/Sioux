// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "pubsub/node.h"
#include "tools/asstring.h"
#include <algorithm>
#include <cstdlib>

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
                const std::string text = tools::as_string(s);
                return key_domain(text.substr(1, text.size()-2));
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

        if ( pos != keys_.end() && !(domain < pos->domain()) )
            result = std::make_pair(true, *pos);

        return result;
    }

    ///////////////////////
    // class node_version
    node_version::node_version()
        : version_(generate_version())
    {
    }

    bool node_version::operator==(const node_version& rhs) const
    {
        return version_ == rhs.version_;
    }

    int node_version::operator-(const node_version& rhs) const
    {
        boost::int_fast64_t distance = 
            static_cast<boost::int_fast64_t>(version_) - static_cast<boost::int_fast64_t>(rhs.version_);

        if ( distance > 0 && distance > std::numeric_limits<int>::max() )
            return std::numeric_limits<int>::max();

        if ( distance < 0 && distance < std::numeric_limits<int>::min() )
            return std::numeric_limits<int>::min();

        return static_cast<int>(distance);
    }

    boost::uint_fast32_t node_version::generate_version()
    {
        return std::rand();
    }

    void node_version::operator-=(unsigned dec)    
    {
        version_ -= dec;
    }

    node_version operator-(node_version start_version, unsigned decrement)
    {
        start_version -= decrement;

        return start_version;
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

    std::pair<bool, json::value> node::update_from(const node_version& known_version) const
    {
        assert(!versions_.empty());
        const int distance = known_version - version_;

        if ( distance <= 0 || distance >= static_cast<int>(versions_.size()) ) 
            return std::make_pair(false, versions_.back());

        return std::make_pair(true, build_comulated_update(distance));
    }

    json::array node::build_comulated_update(unsigned versions) const
    {
        json::array result;
        for ( ; versions != 0; --versions )
            result.add(versions_[versions]);

        return result;
    }

    bool node::operator==(const node& rhs) const
    {
        return versions_ == rhs.versions_ && version_ == rhs.version_;
    }

} // namespace pubsub

