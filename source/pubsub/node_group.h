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

        bool in_group(const node_name&) const;

        /**
         * @brief adds the constrain, that a node_name must have the given domain to be in the group
         * @post in_group() will return false for every group that does not contain the given domain
         */
        node_group& has_domain(const key_domain&);

        /**
         * @brief adds the constrain, that a node_name must habe the given key value to be in the group
         * @post in_group() will return false for every group that does not contain the given key
         */
        node_group& has_key(const key&);

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
    private:
        class impl;

        boost::shared_ptr<impl> pimpl_;

    };
} // namespace pubsub

#endif // include guard



