// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_PUBSUB_NODE_GROUP_H
#define SIOUX_SOURCE_PUBSUB_NODE_GROUP_H

#include <boost/shared_ptr.hpp>

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

        class impl;
    private:
        boost::shared_ptr<impl> pimpl_;
    };

    /**
     * @brief builder for a node group. 
     *
     * The builder class seperates the node_group in a constant and a mutable part. Use this class
     * to configure the node_group and when ready, construct the node_group from 
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
        mutable boost::shared_ptr<node_group::impl> pimpl_;
    };
} // namespace pubsub

#endif // include guard



