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
        root(boost::asio::io_service& io_queue, adapter& adapter, const boost::shared_ptr<const configuration>& default_configuration);

        /**
         * @brief adds or changes the configuration of the given group of nodes
         */
        void add_configuration(const node_group& node_name, const boost::shared_ptr<const configuration>& new_config);
        
        /**
         * @brief removes the named configuration
         * @pre the configuration must have been added by exactly the same node_name
         */
        void remove_configuration(const node_group& node_name);

        /**
         * @brief adds the subscriber to the given node
         */
        void subscribe(subscriber&, const node_name& node_name);

        /**
         * @brief adds the subscriber to the given node. 
         * @param version Tihs is the version that is currently known to the subscriber
         */
        void subscribe(subscriber&, const node_name& node_name, const node_version& version);

        void unsubscribe(const subscriber&, const node_name& node_name);

        void update_node(const node_name& node_name, const json::value& new_data);

        transaction start_transaction();
        void commit(transaction);
        void rollback(transaction);
    };

} // namespace pubsub

#endif // include guard


