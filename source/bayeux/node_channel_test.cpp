// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include <boost/test/unit_test.hpp>

#include "bayeux/node_channel.h"

#include "json/json.h"
#include "pubsub/node.h"

BOOST_AUTO_TEST_CASE( empty_channel_to_node_name_test )
{
	BOOST_CHECK_EQUAL( json::string(), bayeux::channel_from_node_name( pubsub::node_name() ) );
}

static pubsub::node_name node_name( const std::string& text )
{
	const json::object 					obj = json::parse_single_quoted( text ).upcast< json::object >();
	const std::vector< json::string >	keys = obj.keys();

	pubsub::node_name result;
	for ( std::vector< json::string >::const_iterator k = keys.begin(); k != keys.end(); ++k )
	{
		result.add( pubsub::key(
				pubsub::key_domain( k->to_std_string() ),
				obj.at( *k ).upcast< json::string >().to_std_string() ) );
	}

	return result;
}

BOOST_AUTO_TEST_CASE( channel_to_node_name_test )
{
	BOOST_CHECK_EQUAL(
		json::string( "/aasd/bde/cwa/dxy" ),
		bayeux::channel_from_node_name( node_name( "{'p1':'aasd', 'p2':'bde', 'p3':'cwa', 'p4':'dxy'}" ) ) );

	BOOST_CHECK_EQUAL(
		json::string( "/111//333" ),
		bayeux::channel_from_node_name( node_name( "{'p1':'111', 'p2':'', 'p3':'333'}" ) ) );

	BOOST_CHECK_EQUAL(
		json::string( "/a/b" ),
		bayeux::channel_from_node_name( node_name( "{'p2':'b', 'p1':'a'}" ) ) );

	BOOST_CHECK_EQUAL(
		json::string( "/a" ),
		bayeux::channel_from_node_name( node_name( "{'p1':'a'}" ) ) );
}

BOOST_AUTO_TEST_CASE( empty_node_name_to_channel_test )
{
	BOOST_CHECK_EQUAL( bayeux::node_name_from_channel( json::string() ), pubsub::node_name() );
}

BOOST_AUTO_TEST_CASE( node_name_to_channel_test )
{
	BOOST_CHECK_EQUAL(
		bayeux::node_name_from_channel( json::string( "/aasd/bde/cwa/dxy" ) ),
		node_name( "{'p1':'aasd', 'p2':'bde', 'p3':'cwa', 'p4':'dxy'}" ) );

	BOOST_CHECK_EQUAL(
		bayeux::node_name_from_channel( json::string( "/111//333" ) ),
		node_name( "{'p1':'111', 'p2':'', 'p3':'333'}" ) );

	BOOST_CHECK_EQUAL(
		bayeux::node_name_from_channel( json::string( "/a/b" ) ),
		node_name( "{'p2':'b', 'p1':'a'}" ) );

	BOOST_CHECK_EQUAL(
		bayeux::node_name_from_channel( json::string( "/a" ) ),
		node_name( "{'p1':'a'}" ) );
}
