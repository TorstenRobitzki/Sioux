// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef NODE_CHANNEL_H_
#define NODE_CHANNEL_H_


/** @file this file contains functions for transforming between pubsub::node_names and bayeux channels */

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
	json::string channel_from_node_name( const pubsub::node_name& name );
	pubsub::node_name node_name_from_channel( const json::string& channel );
}

#endif /* NODE_CHANNEL_H_ */
