// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef NODE_CHANNEL_H_
#define NODE_CHANNEL_H_


/**
 * @file
 * This file contains functions for transforming between pubsub::node_names and bayeux channels.
 */

namespace json
{
	class string;
}

namespace pubsub
{
	class node_name;
}

namespace bayeux
{
    /**
     * @brief converts a node_name into a bayeux channel name.
     * @sa node_name_from_channel()
     */
	json::string channel_from_node_name( const pubsub::node_name& name );

	/**
	 * @brief converts a bayeux channel to a pubsub::node_name.
	 *
	 * The transformation is done by naming each with / divided section with a new
	 * domain name. As the order of the different sections of a bayeux channel matters,
	 * a running number is used a the domain names. So /a/b/ * becomes:
	 * { "p1" : "a", "p2" : "b", "p3": "*" }
	 *
	 * @post channel == channel_from_node_name( node_name_from_channel( channel ) )
	 * @sa channel_from_node_name()
	 */
	pubsub::node_name node_name_from_channel( const json::string& channel );

	/**
	 * @brief convenience overload of the function above, for testing
	 * @sa pubsub::node_name node_name_from_channel( const json::string& channel )
	 */
    pubsub::node_name node_name_from_channel( const char* );

}

#endif /* NODE_CHANNEL_H_ */
