// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "pubsub/node_group.h"
#include "pubsub/key.h"
#include "pubsub/node.h"
#include <typeinfo>

namespace pubsub
{
    namespace {
        class filter
        {
        public:
            virtual bool in_filter(const node_name&) const = 0;
            virtual ~filter() {}
        };

        class has_domain_filter : public filter
        {
        public:
            explicit has_domain_filter(const key_domain& d) : domain_(d) {}
        private:
            virtual bool in_filter(const node_name& name) const
            {
                return name.find_key(domain_).first;
            }

            const key_domain domain_;
        };

        class has_key_filter : public filter
        {
        public:
            explicit has_key_filter(const key& k) : key_(k) {}
        private:
            virtual bool in_filter(const node_name& name) const
            {
                const std::pair<bool, key> k = name.find_key(key_.domain());

                return k.first && k.second == key_;
            }

            const key key_;
        };
    }

    class node_group::impl
    {
    public:
        impl()
        {
        }

        bool in_group(const node_name& name) const
        {
            filter_list::const_iterator filter = filters_.begin();
            for ( ; filter != filters_.end() && (*filter)->in_filter(name); ++filter )
                ;

            return filter == filters_.end();
        }

        void add_filter(std::auto_ptr<filter> filter)
        {
            filters_.push_back(filter.get());
            filter.release();
        }

        ~impl()
        {
            for ( filter_list::const_iterator i = filters_.begin(); i != filters_.end(); ++i )
                delete *i;
        }
    private:
        impl(const impl&);
        impl& operator=(const impl&);

        typedef std::vector<filter*> filter_list;
        filter_list filters_;
    };


    node_group::node_group()
        : pimpl_(new impl)
    {
    }

    node_group::node_group(const build_node_group& builder)
        : pimpl_(builder.pimpl_)
    {
        builder.pimpl_.reset(new impl);
    }

    bool node_group::in_group(const node_name& name) const
    {
        return pimpl_->in_group(name);
    }

    bool node_group::operator==(const node_group& rhs) const
    {
        return pimpl_.get() == rhs.pimpl_.get();
    }

    bool node_group::operator!=(const node_group& rhs) const
    {
        return !(*this == rhs);
    }

    ///////////////////////////
    // class build_node_group
    build_node_group::build_node_group()
        : pimpl_(new node_group::impl)
    {
    }

    build_node_group& build_node_group::has_domain(const key_domain& d)
    {
        pimpl_->add_filter(std::auto_ptr<filter>(new has_domain_filter(d)));

        return *this;
    }

    build_node_group& build_node_group::has_key(const key& k)
    {
        pimpl_->add_filter(std::auto_ptr<filter>(new has_key_filter(k)));

        return *this;
    }


} // namespace pubsub


