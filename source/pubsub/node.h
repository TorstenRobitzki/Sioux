// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_PUBSUB_NODE_H
#define SIOUX_SOURCE_PUBSUB_NODE_H

#include "pubsub/key.h"
#include "json/json.h"
#include <deque>
#include <iosfwd>
#include <boost/cstdint.hpp>

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

        /**
         * @brief prints the node_name in a human readable manner onto the given stream
         */
        void print(std::ostream&) const;
    private:
        typedef std::vector<key> key_list;
        key_list keys_;
    };

    /**
     * @brief prints the given node_name in a human readable manner onto the given stream
     * @relates node_name
     */
    std::ostream& operator<<(std::ostream& out, const node_name& name);

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

        void operator-=(unsigned);

        /**
         * @brief increments the version and returns itself
         */
        node_version& operator++();

        /**
         * @brief prints the version in a human readable manner onto the given stream
         */
        void print(std::ostream& out) const;
    private:
        boost::uint_fast32_t    version_;

        static boost::uint_fast32_t generate_version();
    }; 

    /**
     * @relates node_version
     */
    std::ostream& operator<<(std::ostream& out, const node_version&);

    /**
     * @brief calculates the version, that was decrement versions younger than start_version
     * @related node_version
     */
    node_version operator-(node_version start_version, unsigned decrement);

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
        std::pair<bool, json::value> get_update_from(const node_version& known_version) const;

        /**
         * @brief changes the current nodes data and increments the current version.
         *
         * The node keeps updates from the old data version to the new data version till a 
         * certain level of size for the updates is reached.
         * 
         * If new_data is equal to data() no action is performed and the function returns false.
         *
         * @param new_data the new data of the node
         * @param keep_update_size_percent the maximum, total size of updates to keep 
         *                                 expressed as a percentage of the new_data size
         * @return true, if the new data is different to the currently stored data.
         * @post data() will return new_data
         * @post current_version() will be incremented if data() != new_data
         */
        bool update(const json::value& new_data, unsigned keep_update_size_percent);

    private:
        void remove_old_versions(std::size_t max_size);

        json::value     data_;
        node_version    version_;
        json::array     updates_;
    };


}

#endif // include guard


