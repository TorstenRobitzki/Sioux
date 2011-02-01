// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "pubsub/node.h"
#include "tools/asstring.h"
#include "json/delta.h"
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

    void node_name::print(std::ostream& out) const
    {
        out << "{";

        for ( key_list::const_iterator i = keys_.begin(); i != keys_.end(); ++i )
        {
            out << *i;
            if ( i+1 != keys_.end() )
                out << ", ";
        }

        out << "}";
    }

    std::ostream& operator<<(std::ostream& out, const node_name& name)
    {   
        name.print(out);
        return out;
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

    void node_version::print(std::ostream& out) const
    {
        out << version_;
    }

    boost::uint_fast32_t node_version::generate_version()
    {
        return std::rand();
    }

    void node_version::operator-=(unsigned dec)    
    {
        version_ -= dec;
    }

    node_version& node_version::operator++()
    {
        ++version_;
        return *this;
    }

    node_version operator-(node_version start_version, unsigned decrement)
    {
        start_version -= decrement;

        return start_version;
    }

    std::ostream& operator<<(std::ostream& out, const node_version& v)
    {
        v.print(out);
        return out;
    }

    ///////////////
    // class node
    node::node(const node_version& first_version, const json::value& first_versions_data)
        : data_(first_versions_data)
        , version_(first_version)
        , updates_()
    {
    }

    node_version node::current_version() const
    {
        return version_;
    }

    node_version node::oldest_version() const
    {
        return version_ - updates_.length();
    }

    const json::value& node::data() const
    {
        return data_;
    }

    std::pair<bool, json::value> node::get_update_from(const node_version& known_version) const
    {
        const int distance = version_ - known_version;

        if ( distance <= 0 || distance > static_cast<int>(updates_.length()) ) 
            return std::make_pair(false, data_);

        return std::make_pair(true, json::array(updates_, distance, updates_.length() - distance));
    }

    void node::update(const json::value& new_data, unsigned keep_update_size_percent)
    {
        if ( new_data == data_ )
            return;

        const std::size_t max_size = new_data.size() * keep_update_size_percent / 100;

        if ( max_size != 0 )
        {
            std::pair<bool, json::value> update_instruction = delta(data_, new_data, max_size);

            if ( update_instruction.first )
                updates_.insert(updates_.length(), update_instruction.second);

        }

        data_ = new_data;
        ++version_;

        // remove oldes versions until the max_size reached
        while ( !updates_.empty() && updates_.size() > max_size )
            updates_.erase(0, 1u);
    }
} // namespace pubsub

