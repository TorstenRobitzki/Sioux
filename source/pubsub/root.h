// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_PUBSUB_ROOT_H
#define SIOUX_SOURCE_PUBSUB_ROOT_H

#include <boost/shared_ptr.hpp>

namespace boost {
    namespace asio {
        class io_service;
    }
}

namespace json {
    class value;
}

namespace pubsub
{
    class adapter;
    class configuration;
    class node_group;
    class node_name;
    class node_version;
    class subscriber;
    class transaction;

    /**
     * @brief root of a changeable and observable tree like data structure
     */
    class root
    {
    public:
        /**
         * @brief contructs a root from certain user defined settings.
         *
         * @param io_queue queue that is used to perform asynchronouse io operations
         * @param adapter user defined adapter to define aspects like authorization and validation
         * @param default_configuration a default configuration to be used for all node that do not have a different configuration defined
         */
        root(boost::asio::io_service& io_queue, adapter& adapter, const configuration& default_configuration);

        /**
         * @brief adds or changes the configuration of the given group of nodes
         *
         * The new_configuration is added at the the end of the list of configurations. For every new node,
         * this list is searched for an entry where the name of the node fits with a given node_group. If an entry
         * is found, the stored configuration is applied to the new node. If no entry is found, the default configuration 
         * passed to the c'tor is used.
         */
        void add_configuration(const node_group& node_name, const configuration& new_config);
        
        /**
         * @brief removes the named configuration
         * @pre the configuration must have been added by exactly the same node_name
         */
        void remove_configuration(const node_group& node_name);

        /**
         * @brief adds the subscriber to the given node
         */
        void subscribe(boost::shared_ptr<subscriber>&, const node_name& node_name);

        /**
         * @brief adds the subscriber to the given node. 
         * @param version This is the version that is currently known to the subscriber
         */
        void subscribe(boost::shared_ptr<subscriber>&, const node_name& name, const node_version& version);

        /**
         * @brief stops the subscription of the subscriber to the named node.
         */
        void unsubscribe(const boost::shared_ptr<subscriber>&, const node_name& node_name);

        /**
         * @brief stops all subscriptions of the subscriber
         */
        void unsubscribe_all(const boost::shared_ptr<subscriber>&);

        void update_node(const node_name& node_name, const json::value& new_data);

    private:
        // no copy, no assigment; not implemted
        root(const root&);
        root& operator=(const root&);

        class impl;
        impl*   pimpl_;
    };

} // namespace pubsub

#endif // include guard


