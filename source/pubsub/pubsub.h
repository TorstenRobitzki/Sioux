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

namespace pubsub
{
    class node;
    class node_version;
    class node_name;

    /*
    - Hohe Anzahl Subscriber auf einen Knoten
        - vielleicht Gruppierung von Subscribern
    - Hohe Knoten-Zahl bei niedriger Anzahl Subscriber (~1 Subscriber pro Knoten)
    - Sehr hohe Updaterate der Knoten
        - delay von updates
    - Anzahl Schlüssel 2-10
    - Partitionierung 
        - durch Bildung von Bereichen von Schlüsseln
        - für die Bildung von Subscriber-Gruppen
        - für die Partitionierung der Daten auf andere Rechner
    - hierarchische Struktur
        - Um defaults für Kind-Knoten aus Eltern-Knoten zu generieren.
        - Subscriben auf mehrere Knoten
    - Operationen:

    */
    /*
    Prüfungen:
    - Sind Schlüssel-Namen gültig?
    - Sind Schlüssel-Werte im gültigen Bereich?
    - Sind Schlüssel-Werte erlaubt?
    - Sind Schlüssel vollständig?
    - Sind Schlüssel aus der Session zu ermitteln?
    */


    
    /**
     * @brief describes update policy, node timeout
     */
    class configuration
    {
    public:
        /**
         * @brief the time, that a node without subscriber should stay in the data model
         */
        boost::posix_time::time_duration    node_timeout() const;

        /**
         * @brief The time that have to elapse, before a new version of a document will
         *        be published. 
         *
         * If at the time, where the update was made, the time isn't elapsed, the update
         * will be published, when the time elapses.
         */
        boost::posix_time::time_duration    min_update_period() const;

        /**
         * @brief the ratio of update costs to full document size in %
         */
        unsigned max_update_size() const;
    };

    /**
     * @brief interface for a subcriber / receiver of updates 
     */
    class subscriber
    {
    public:
        /**
         * @brief will be call, when the subscriber is registered for notifications,
         *        and the nodes data was changed.
         */
        virtual void on_udate(const node_name& name, const node& data) = 0;

        virtual ~subscriber() {}
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
        virtual bool valid_node(const node_name& node_name) = 0;

        /**
         * @brief returns the initial value of the node 
         *
         * Asynchronous
         */
        virtual json::value node_init(const node_name& node_name) = 0;

        /**
         * @brief returns true, if the given subscriber is authorized to subscribe to the named node
         */
        virtual bool authorize(const subscriber&, const node_name& node_name) = 0;

        virtual ~adapter() {}
    };

    class transaction
    {
    };

} // namespace data

#endif // include guard


