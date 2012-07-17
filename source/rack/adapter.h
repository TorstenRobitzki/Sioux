// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef RACK_ADAPTER_H
#define RACK_ADAPTER_H

#include "pubsub/pubsub.h"
#include <ruby.h>

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

        void validate_node(const pubsub::node_name& node_name, const boost::shared_ptr< pubsub::validation_call_back >&);
        void authorize(const boost::shared_ptr< pubsub::subscriber >&, const pubsub::node_name& node_name,
            const boost::shared_ptr< pubsub::authorization_call_back >&);
        void node_init(const pubsub::node_name& node_name, const boost::shared_ptr< pubsub::initialization_call_back >&);

    private:
        adapter( const adapter& );
        adapter& operator=( const adapter& );

        const VALUE         adapter_;
        ruby_land_queue&    ruby_land_;
    };
}


#endif




