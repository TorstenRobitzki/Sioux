#ifndef SIOUX_PUBSUB_LOGGING_ADAPTER_H
#define SIOUX_PUBSUB_LOGGING_ADAPTER_H

#include "pubsub/pubsub.h"

namespace pubsub {

class logging_adapter : public adapter
{
public:
    explicit logging_adapter( adapter& );
private:
    virtual void validate_node(const node_name& node_name, const boost::shared_ptr<validation_call_back>&);
    virtual void authorize(const boost::shared_ptr<subscriber>&, const node_name& node_name, const boost::shared_ptr<authorization_call_back>&);
    virtual void node_init(const node_name& node_name, const boost::shared_ptr<initialization_call_back>&);
    virtual void invalid_node_subscription(const node_name& node, const boost::shared_ptr<subscriber>&);
    virtual void unauthorized_subscription(const node_name& node, const boost::shared_ptr<subscriber>&);
    virtual void initialization_failed(const node_name& node);

    adapter& adapter_;
};

}

#endif

