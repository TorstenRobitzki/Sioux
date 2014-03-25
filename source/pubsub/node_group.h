// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_PUBSUB_NODE_GROUP_H
#define SIOUX_SOURCE_PUBSUB_NODE_GROUP_H

#include <boost/shared_ptr.hpp>
#include <iosfwd>
#include <vector>

namespace pubsub
{
    class node_name;
    class key;
    class key_domain;
    class build_node_group;

    /**
     * @brief a group of nodes is identified by some of there keys and by ranges of keys
     * 
     * If a node has the key_domains "location, product, company", a node_group can name all of the nodes
     * that have the key_domains "location, product" and from that only specific keys values.
     *
     * To configure a node_group a build named build_node_group in combination with a set of free functions 
     * is used.
     */
    class node_group
    {
    public:
        /**
         * @brief a default node_group
         * @post in_group() will return true in any case
         */
        node_group();

        /**
         * @brief constructs a node_group from a node_group builder
         * @sa build_node_group
         */
        node_group(const build_node_group&);

        bool in_group(const node_name&) const;

        /**
         * @brief returns true, if the rhs node_group is constructed or copied from the very same
         *        node_group. 
         */
        bool operator==(const node_group& rhs) const;

        /**
         * @brief returns !(*this ==  rhs)
         * @sa operator==
         */
        bool operator!=(const node_group& rhs) const;

        /**
         * @brief prints the group in a human readable manner onto the given stream
         */
        void print(std::ostream& out) const;

        class impl;
        class filtered_impl;
        explicit node_group( const boost::shared_ptr<impl>& );
    private:
        boost::shared_ptr<impl> pimpl_;
    };

    /**
     * @brief prints the group in a human readable manner onto the given stream
     * @relates node_group
     */
    std::ostream& operator<<(std::ostream& out, const node_group& group);

    /**
     * @brief builder for a node group. 
     *
     * The builder class seperates the node_group in a constant and a mutable part. Use this class
     * to configure the node_group and when ready, construct the node_group from .
     *
     * For every mutator function, there is a free function with the very same name, that constructs
     * a build_node_group. So instead of calling build_node_group().has_domain(d).has_key(k), 
     * has_domain(d).has_key(k) can be used to configure the very same node_group.
     * @relates node_group
     */
    class build_node_group
    {
    public:
        build_node_group();

        /**
         * @brief adds the constrain, that a node_name must have the given domain to be in the group
         * @post in_group() will return false for every group that does not contain the given domain
         */
        build_node_group& has_domain(const key_domain&);

        /**
         * @brief adds the constrain, that a node_name must habe the given key value to be in the group
         * @post in_group() will return false for every group that does not contain the given key
         */
        build_node_group& has_key(const key&);

    private:
        friend class node_group;
        mutable boost::shared_ptr<node_group::filtered_impl> pimpl_;
    };

    /**
     * @brief constructs a build_node_group, calls has_domain() on it and returns the result
     * @relates build_node_group
     */
    build_node_group has_domain(const key_domain&);

    /**
     * @brief constructs a build_node_group, calls has_key() on it and returns the result
     * @relates build_node_group
     */
    build_node_group has_key(const key&);

    /**
     * @brief returns a set of node_groups that equaly distributes the key space among all groups
     * @post equaly_distributed_node_groups( N ).size() == N
     * @pre number_of_groups != 0
     */
    std::vector< node_group > equaly_distributed_node_groups( unsigned number_of_groups );

} // namespace pubsub

#endif // include guard



