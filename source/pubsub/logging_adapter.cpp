#include "pubsub/logging_adapter.h"

namespace pubsub {

    logging_adapter::logging_adapter( adapter& a ) : adapter_( a )
    {
    }
    
    void logging_adapter::validate_node( const node_name& node_name, const boost::shared_ptr<validation_call_back>& cb )
    {
        adapter_.validate_node( node_name, cb );
    }    
    
    void logging_adapter::authorize( const boost::shared_ptr<subscriber>& subscriber, const node_name& node_name, const boost::shared_ptr<authorization_call_back>& cb )
    {
        adapter_.authorize( subscriber, node_name, cb );
    }
    
    void logging_adapter::node_init( const node_name& node_name, const boost::shared_ptr<initialization_call_back>& cb )
    {
        adapter_.node_init( node_name, cb );
    }
    
    void logging_adapter::invalid_node_subscription( const node_name& node, const boost::shared_ptr<subscriber>& subscriber )
    {
        adapter_.invalid_node_subscription( node, subscriber );
    }
    
    void logging_adapter::unauthorized_subscription( const node_name& node, const boost::shared_ptr<subscriber>& subscriber )
    {
        adapter_.unauthorized_subscription( node, subscriber );
    }
    
    void logging_adapter::initialization_failed( const node_name& node )
    {
        adapter_.initialization_failed( node );
    }
}