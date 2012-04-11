// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_DATA_DOCUMENT_H
#define SIOUX_SOURCE_DATA_DOCUMENT_H

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/asio/io_service.hpp>


namespace json {
    class value;
}

/** @namespace pubsub */
namespace pubsub
{ 
    class node;
    class node_version;
    class node_name;
    class root;
    /*
    - Hohe Anzahl Subscriber auf einen Knoten
        - vielleicht Gruppierung von Subscribern
    - Hohe Knoten-Zahl bei niedriger Anzahl Subscriber (~1 Subscriber pro Knoten)
    - Sehr hohe Updaterate der Knoten
        - delay von updates
    - Anzahl Schlssel 2-10
    - Partitionierung 
        - durch Bildung von Bereichen von Schlsseln
        - fr die Bildung von Subscriber-Gruppen
        - fr die Partitionierung der Daten auf andere Rechner
    - hierarchische Struktur
        - Um defaults fr Kind-Knoten aus Eltern-Knoten zu generieren.
        - Subscriben auf mehrere Knoten
    - Operationen:

    */
    /*
    Prfungen:
    - Sind Schlssel-Namen gltig?
    - Sind Schlssel-Werte im gltigen Bereich?
    - Sind Schlssel-Werte erlaubt?
    - Sind Schlssel vollstndig?
    - Sind Schlssel aus der Session zu ermitteln?
    */


    
    /**
     * @brief interface for a subscriber / receiver of updates
     */
    class subscriber
    {
    public:
        /**
         * @brief will be call, when the subscriber is registered for notifications,
         *        and the nodes data was changed.
         */
        virtual void on_update(const node_name& name, const node& data) = 0;

        /**
         * @brief will be called, when a subscription was done to an invalid node.
         *
         * This default implementation does nothing.
         */
        virtual void on_invalid_node_subscription(const node_name& node);

        /**
         * @brief will be called, when the authorization to a subscribed node failed.
         *
         * This default implementation does nothing.
         */
        virtual void on_unauthorized_node_subscription(const node_name& node);

        /**
         * @brief will be called, when the initialization of a node that this subscriber subscribed
         *        to, failed. This default implementation does nothing.
         */
        virtual void on_failed_node_subscription(const node_name& node);

        virtual ~subscriber() {}
    };

    /**
     * @brief interface, that will be implemented by the pubsub library to provide
     *        validation call back.
     */
    class validation_call_back
    {
    public:
        /**
         * @brief to be call, if the node_name is valid
         */
        virtual void is_valid() = 0;

        /**
         * @brief to be call, if a node_name is not valid
         */
        virtual void not_valid() = 0;

        virtual ~validation_call_back() {}
    };

    /**
     * @brief interface, that will be implmented by the pubsub library to provide
     *        a call back for asynchronous node initialization
     */
    class initialization_call_back
    {
    public:
        /**
         * @brief to be called, when a nodes initial value is ready.
         */
        virtual void initial_value(const json::value&) = 0;

        virtual ~initialization_call_back() {}
    };

    /**
     * @brief interface, that will be implemented by the pubsub library to provide
     *        a call back for asynchronous authorization.
     */
    class authorization_call_back
    {
    public:
        /**
         * @brief to be called, if the requesting subscriber is authorized to access the requested node
         */
        virtual void is_authorized() = 0;

        /**
         * @brief to be called, when the subscriber is not authorized to access the requested node
         */
        virtual void not_authorized() = 0;

        virtual ~authorization_call_back() {}
    };

    /**
     * @brief application interface
     */
    class adapter
    {
    public:
        /**
         * @brief returns true, if the given node_name names a valid node
         */
        virtual void validate_node(const node_name& node_name, const boost::shared_ptr<validation_call_back>&) = 0;

        /**
         * @brief returns true, if the given subscriber is authorized to subscribe to the named node
         */
        virtual void authorize(const boost::shared_ptr<subscriber>&, const node_name& node_name, const boost::shared_ptr<authorization_call_back>&) = 0;

        /**
         * @brief will be called, when node initialization is required
         *
         * When the data of the node is known, update_node() should be called
         * on the passed target.
         * @sa initialization_failed()
         */
        virtual void node_init(const node_name& node_name, const boost::shared_ptr<initialization_call_back>&) = 0;

        /**
         * @brief will be called, when ever a subscriber tried to subscribe to an invalid node
         *
         * The default implementation does nothing. The passed subscriber will not be further accessed,
         * and can be deleted.
         */
        virtual void invalid_node_subscription(const node_name& node, const boost::shared_ptr<subscriber>&);

        /**
         * @brief will be called, when ever a subscriber tried to subscribe to a node and is not authorized to do so.
         *
         * The default implementation does nothing. The passed subscriber will not be further accessed,
         * and can be deleted.
         */
        virtual void unauthorized_subscription(const node_name& node, const boost::shared_ptr<subscriber>&);

        /**
         * @brief will be called, when ever the initialization of a node failed.
         *
         * When validation and authorization was done successfully or was not necessary, but the callback passed to node_init()
         * was not called and no copy of the passed reference was made.
         */
        virtual void initialization_failed(const node_name& node);

        virtual ~adapter() {}
    };

} // namespace pubsub

#endif // include guard


