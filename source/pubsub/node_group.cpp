// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "pubsub/node_group.h"
#include "pubsub/key.h"
#include "pubsub/node.h"
#include <typeinfo>
#include <ostream>

namespace pubsub
{
    namespace {
        class filter
        {
        public:
            virtual bool in_filter(const node_name&) const = 0;
            virtual void print(std::ostream&) const = 0;
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

            virtual void print(std::ostream& out) const 
            {
                out << "has_domain(" << domain_ << ")";
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

            virtual void print(std::ostream& out) const 
            {
                out << "has_key(" << key_ << ")";
            }

            const key key_;
        };
    }

    class node_group::impl
    {
    public:
        virtual bool in_group(const node_name& name) const = 0;
        virtual void print(std::ostream& out) const = 0;

        virtual ~impl() {}
    };

    class node_group::filtered_impl : public node_group::impl
    {
    public:
        filtered_impl()
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
            assert( filter.get() );
            filters_.push_back(filter.get());
            filter.release();
        }

        ~filtered_impl()
        {
            for ( filter_list::const_iterator i = filters_.begin(); i != filters_.end(); ++i )
                delete *i;
        }

        void print(std::ostream& out) const
        {
            for ( filter_list::const_iterator i = filters_.begin(); i != filters_.end(); ++i )
            {
                (*i)->print(out);
                if ( i+1 != filters_.end() )
                    out << '.';
            }
        }

    private:
        filtered_impl(const filtered_impl&);
        filtered_impl& operator=(const filtered_impl&);

        typedef std::vector<filter*> filter_list;
        filter_list filters_;
    };


    node_group::node_group()
        : pimpl_(new filtered_impl)
    {
    }

    node_group::node_group(const build_node_group& builder)
        : pimpl_(builder.pimpl_)
    {
        assert( pimpl_.get() );
        builder.pimpl_.reset(new filtered_impl);
    }

    node_group::node_group( const boost::shared_ptr<impl>& p )
        : pimpl_( p )
    {
        assert( p.get() );
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

    void node_group::print(std::ostream& out) const
    {
        return pimpl_->print(out);
    }

    std::ostream& operator<<(std::ostream& out, const node_group& group)
    {
        group.print(out);
        return out;
    }

    ///////////////////////////
    // class build_node_group
    build_node_group::build_node_group()
        : pimpl_(new node_group::filtered_impl)
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

    build_node_group has_domain(const key_domain& d)
    {
        return build_node_group().has_domain(d);
    }

    build_node_group has_key(const key& k)
    {
        return build_node_group().has_key(k);
    }

    namespace {
        struct mod_filter : node_group::impl
        {
            mod_filter( unsigned n, unsigned mod ) : n_( n ), mod_( mod ) {}

            unsigned n_, mod_;

            bool in_group(const node_name&) const
            {
                return true;
            }

            void print( std::ostream& o ) const
            {
                o << "f(key) % " << mod_ << " == " << n_;
            }
        };
    }

    std::vector< node_group > equaly_distributed_node_groups( unsigned number_of_groups )
    {
        assert( number_of_groups );
        std::vector< node_group > result;

        for ( unsigned n = 0 ; n != number_of_groups; ++n  )
            result.push_back( node_group( boost::shared_ptr< node_group::impl >( new mod_filter( n, number_of_groups ) ) ) );

        return result;
    }

} // namespace pubsub


