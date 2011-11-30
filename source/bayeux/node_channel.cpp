// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "bayeux/node_channel.h"

#include "pubsub/node.h"
#include "tools/asstring.h"
#include "tools/split.h"
#include "tools/substring.h"

#include <algorithm>

namespace bayeux
{
	static const char path_split = '/';

	json::string channel_from_node_name( const pubsub::node_name& name )
	{
		pubsub::node_name::key_list keys = name.keys();
		std::sort( keys.begin(), keys.end() );

		std::string result;

		for ( pubsub::node_name::key_list::const_iterator k = keys.begin(); k != keys.end(); ++k )
			result = result + path_split + k->value();

		return json::string( result.c_str() );
	}

	template < class String >
	static void add_key( pubsub::node_name& result, unsigned parameter_name, String& parameter_value )
	{
		const pubsub::key new_key(
				pubsub::key_domain( "p" + tools::as_string( parameter_name ) ),
				std::string( parameter_value.begin(), parameter_value.end() ) );
		result.add( new_key );
	}

	pubsub::node_name node_name_from_channel( const json::string& channel_name )
	{
		pubsub::node_name result;

		typedef tools::basic_substring< std::string::const_iterator > substring;

		const std::string 	value = channel_name.to_std_string();
		substring			channel( value.begin(), value.end() );
		substring			empty_start;

 		if ( tools::split_to_empty( channel, path_split, empty_start, channel ) && empty_start.empty() )
		{
 			unsigned	parameter_name = 1;
			substring	parameter_value;

			for ( ; tools::split_to_empty( channel, path_split, parameter_value, channel ); ++parameter_name )
				add_key( result, parameter_name, parameter_value );

			add_key( result, parameter_name, channel );
		}

		return result;
	}
}
