#ifndef RACK_ADAPTER_H
#define RACK_ADAPTER_H

#include "pubsub/pubsub.h"
#include "http/http.h"
#include <ruby.h>
#include <utility>

namespace json {
    class value;
    class string;
}

namespace rack
{
    class ruby_land_queue;

    /**
     * @brief implementation of the pubsub::adapter interface, that forwards all request to a given ruby object
     */
    class adapter : public pubsub::adapter
    {
    public:
        adapter( VALUE ruby_adapter, ruby_land_queue& );

        // publish version for bayeux:
        // @todo move call to adapter.publish from bayeux.cpp

        // publish version for pubsub:
        typedef std::pair< json::value, http::http_error_code > pubsub_publish_result_t;

        pubsub_publish_result_t publish( const json::value& body, VALUE root );

    private:
        void validate_node(const pubsub::node_name& node_name, const boost::shared_ptr< pubsub::validation_call_back >&);
        void authorize(const boost::shared_ptr< pubsub::subscriber >&, const pubsub::node_name& node_name,
            const boost::shared_ptr< pubsub::authorization_call_back >&);
        void node_init(const pubsub::node_name& node_name, const boost::shared_ptr< pubsub::initialization_call_back >&);

        virtual void invalid_node_subscription(const pubsub::node_name& node, const boost::shared_ptr<pubsub::subscriber>&);
        virtual void unauthorized_subscription(const pubsub::node_name& node, const boost::shared_ptr<pubsub::subscriber>&);
        virtual void initialization_failed(const pubsub::node_name& node);

        adapter( const adapter& );
        adapter& operator=( const adapter& );

        const VALUE         adapter_;
        ruby_land_queue&    ruby_land_;
    };
}


#endif




