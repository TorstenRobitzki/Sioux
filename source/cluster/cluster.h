/*
- partitioning
    - Summe aller Partitionen muss der gesammte Schlüssel-raum sein
    - Partitionen dürfen sich nicht überschneiden
    - der gesammte Schlüssel-Raum ist unbekannt
    - einfache "beliebige" clusterung über checksumme des keys modulo Anzahl benötigter Partitionen
(- caching
    - ein cluster member kann die Partition eines anderen Members lesend cachen
    - ein cluster member ohne eigene Partition kann als read cache funktionieren)
- failover
    - gibt es mehr als 2 cluster member und fällt ein member aus, könnte die Mehrheit abstimmen, ob der Member ausgefallen ist.
    - bei Ausfall übernimmt ein member die partition des ausgefallenen members
- visibility of cluster members
    - members are configured
    - adding new members is simply a configuration change
- reconfiguration
- cryptography
- pubsub
    - valid
        - kann local oder beim zuständigen remote cluster member geschehen oder remote abgelehnt werden, je nach Konfiguration
    - authorized
        - dito
    - init
        - dito
    - update
        - dito
        - "Operational Transformation" könnte verwendet werden, um updates von mehreren cluster membern zu erlauben
    - subscribe
        - wenn node_name in der partition -> bingo
        -
    - add_configuration
    - remove_configuration

- rpc
    - boost::serialization + function/implementation registration

Lösung:
    - ein cluster besteht aus einer festen Anzahl Partitionen
    - ein cluster besteht aus einer festen Anzahl member
    - jedem Member ist einer Partition zugeordnet
    - jedem Member ist ein Fail-Over Member zugeordnet
    - fällt ein Member aus, übernimmt der Fail-Over member die zugeordnete Partition des ausgefallenen Members
    - jeder member implementiert das root-interface in dem

    - wie einigt sich der cluster darauf, dass ein Member ausgefallen ist?
        - jeder member ist mit jedem anderen member verbunden
*/

namespace cluster
{
    /**
     * @brief a set of node names
     *
     * A partition implementation can basicly answer the question whether a node_name os part of the partition or not
     */
    class partition
    {
    public:
        bool operator()( const pubsub::node_name& ) const;

    };

    /**
     * @brief configuration data, that keeps track of the patitions assigments to cluster members
     *
     * It's vital that this part of the configuration is the same for all cluster members.
     */
    class member_partition_assignments
    {
    public:
        /**
         * returns the partition assigned to the given cluster member
         */
        partition assigned_partition( const std::string& cluster_member );

        /**
         * returns the list of cluster members that the given member have to take over. If an instance of the
         * returned list failed, the given member have to handle there partition too.
         */
        const std::vector< std::string >& failover_members( const std::string& cluster_member );


    };

    /**
     * @brief keeps track of all relevant cluster configurations
     */
    class configuration
    {
    public:
        const member_partion_assignments& members() const;
    };

    class root_factory;

    class cluster_root : public pubsub::root_interface
    {
    public:
        cluster_root( const std::string& cluster_name, configuration&, root_factory&,
            boost::asio::io_service& io_queue, pubsub::adapter& adapter, const pubsub::configuration& default_configuration);
    private:
        virtual void subscribe( const boost::shared_ptr< pubsub::subscriber >&, const pubsub::node_name& node_name);
        virtual bool unsubscribe( const boost::shared_ptr< pubsub::subscriber >&, const pubsub::node_name& node_name);
        virtual unsigned unsubscribe_all( const boost::shared_ptr< pubsub::subscriber >& );
        virtual void update_node( const pubsub::node_name& node_name, const json::value& new_data );

        typedef std::pair< partition, boost::shared_ptr< pubsub::root_interface > > partition_to_root_t;
        typedef std::vector< partition_to_root_t > root_list_t;

    };
}
