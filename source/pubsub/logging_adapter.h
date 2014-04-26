#ifndef SIOUX_PUBSUB_LOGGING_ADAPTER_H
#define SIOUX_PUBSUB_LOGGING_ADAPTER_H

#include "pubsub/pubsub.h"
#include <iosfwd>

namespace pubsub {

    /**
     * @brief pubsub::adapter implementation, that logs all requests to an adpater and there results and forward them to an other adapter implemention.
     *
     * All calls are forwared to the contructor given adapter and logged, when the result of the call is provided by a call to the given 
     * call_back. If a validation_call_back or is skipped (none of the provided functions are called), a result of false is logged. For
     * a skipped initialization_call_back, no result is logged. The logging_adapter writes a json array to the given stream. The closing brace 
     * is written by the destrutor. 
     *
     * For every function call, one json object is logged, containing the function name, the node_name and the result.
     *
     * Example:
     *  [
     *     { "call": "validate_node", "node": {"a": 1, "b": 2}, "result": true },
     *     { "call": "authorize", "node": {"a": 1, "b": 2}, "result": true },
     *     { "call": "node_init", "node": {"a": 1, "b": 2}, "result": [1,2,3,4] }
     *  ]
     */
    class logging_adapter : public adapter
    {
    public:
        logging_adapter( adapter& wrapped, std::ostream& log );
        ~logging_adapter(); // no throw
    private:
        virtual void validate_node(const node_name& node_name, const boost::shared_ptr< validation_call_back >&);
        virtual void authorize(const boost::shared_ptr<subscriber>&, const node_name& node_name, const boost::shared_ptr< authorization_call_back >&);
        virtual void node_init(const node_name& node_name, const boost::shared_ptr< initialization_call_back >&);

        void next_entry();
        
        adapter&        adapter_;
        std::ostream&   log_;
        bool            first_;
    };

}

#endif

