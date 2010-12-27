// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_PUBSUB_NODE_H
#define SIOUX_SOURCE_PUBSUB_NODE_H

#include "pubsub/key.h"
#include "json/json.h"
#include <deque>


namespace pubsub
{
    /**
     * @brief a node_name is a complete list of keys to address a single node
     */
    class node_name
    {
    public:
        /** 
         * @brief a default node_name, that compares equal to any other default constructed
         *        node_name.
         */
        node_name();

        /**
         * @brief constructs a list of keys and values from a json::object
         *
         * The main purpose for this c'tor is testing.
         */
        explicit node_name(const json::object&);

        bool operator==(const node_name& rhs) const;

        bool operator<(const node_name& rhs) const;

        std::pair<bool, key> find_key(const key_domain&) const;
    private:
        typedef std::vector<key> key_list;
        key_list keys_;
    };

    /**
     * @brief version of a node
     */
    class node_version
    {
    public:
        /**
         * @brief first, initial version of a document
         */
        node_version();

        /**
         * @brief returns true, if this and rhs are the same versions
         */
        bool operator==(const node_version& rhs) const;

        /**
         * @brief calculates the distance between two versions
         *
         * If the returned value is 0, both versions are equal. If the return
         * value is negativ, this version is older than the right hand side argument.
         */
        int operator-(const node_version& rhs) const;

        node_version operator-(unsigned) const;
    }; 

    /**
     * @brief repositiory of node data and possible updates between versions
     *
     * The responsibility of this class is to keep the nodes data with the nodes data
     * version and possible existing updates from older versions to the current version.
     */
    class node
    {
    public:
        /** 
         * @brief constructs the node from a current version and it's data
         */
        node(const node_version& first_version, const json::value& first_versions_data);

        node_version current_version() const;
        node_version oldest_version() const;

        const json::value& data() const;

        /**
         * @brief returns an update that will update the nodes data from the given, known version
         *        to the current version of this node. 
         *
         * If a delta between the current version and known_version is deliverable, the first member
         * of the returned pair is true and the second member contains an array with update operations,
         * that can be passed to json::update(). If such an update is unknown, the first member will be 
         * false and the second member will contain the current data
         */
        std::pair<bool, json::value> update_from(const node_version& known_version) const;

        /** 
         * @brief compare two nodes for equal data, and versions
         *
         * The purpose of this function is testing. It's expected to be not a cheap operation.
         */
        bool operator==(const node& rhs) const;
        
    private:
        std::deque<json::value> versions_;
        node_version            version_;
    };


}

#endif // include guard


