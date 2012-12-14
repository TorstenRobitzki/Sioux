/*
- partitioning
    - Summe aller Partitionen muss der gesammte Schlüssel-raum sein
    - Partitionen dürfen sich nicht überschneiden
    - der gesammte Schlüssel-Raum ist unbekannt
    - einfache "beliebige" clusterung über checksumme des keys modulo Anzahl benötigter Partitionen
- caching
    - ein cluster member kann die Partition eines anderen Members lesend cachen
    - ein cluster member ohne eigene Partition kann als read cache funktionieren
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
