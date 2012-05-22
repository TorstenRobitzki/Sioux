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
    class subscriber;

    /**
     * @brief root of a changeable and observable tree like data structure
     *
     * In some circumstances there might be race conditions, when it comes to subscribing and unsubscribing the
     * same subscriber to / from the same node. It's important for the overall effect that this two operations are
     * performed in the right order. If this operations are performed at the same time, the root object can not decide
     * which effect is the intended one. So it's up to the caller to make sure that subscribe() and unsubscribe() are
     * called in the right and intended order.
     */
    class root
    {
    public:
        /**
         * @brief contructs a root from certain user defined settings.
         *
         * @param io_queue queue that is used to perform asynchronouse io operations
         * @param adapter user defined adapter to define aspects like authorization and validation
         * @param default_configuration a default configuration to be used for all node that do not have a different
         *                              configuration defined
         */
        root(boost::asio::io_service& io_queue, adapter& adapter, const configuration& default_configuration);

        /**
         * @brief adds or changes the configuration of the given group of nodes
         *
         * The new_configuration is added at the the end of the list of configurations. For every new node,
         * this list is searched for an entry where the name of the node fits with a given node_group. If an entry
         * is found, the stored configuration is applied to the new node. If no entry is found, the default
         * configuration passed to the c'tor is used.
         */
        void add_configuration(const node_group& node_name, const configuration& new_config);
        
        /**
         * @brief removes the named configuration
         * @pre the configuration must have been added by exactly the same node_name
         */
        void remove_configuration(const node_group& node_name);

        /**
         * @brief adds the subscriber to the given node
         *
         * The subscriber will be notified with a call to on_update() when the data of the given node changes and
         * when the subscription was successful.
         */
        void subscribe(const boost::shared_ptr<subscriber>&, const node_name& node_name);

        /**
         * @brief stops the subscription of the subscriber to the named node.
         * @return returns true, if the subscriber was subscribed to the named node.
         */
        bool unsubscribe(const boost::shared_ptr<subscriber>&, const node_name& node_name);

        /**
         * @brief stops all subscriptions of the subscriber
         * @return returns the number of subjects the subscriber was unsubscribed from
         */
        unsigned unsubscribe_all(const boost::shared_ptr<subscriber>&);

        /**
         * @brief updates the named node to a new value
         * @attention it's important to know that authorization control doesn't apply to this function. The function
         *            should only be called if whom ever triggered the data change, was authorized to do so.
         */
        void update_node(const node_name& node_name, const json::value& new_data);

    private:
        // no copy, no assignment; not implemented
        root(const root&);
        root& operator=(const root&);

        class impl;
        impl*   pimpl_;
    };

} // namespace pubsub

#endif // include guard


