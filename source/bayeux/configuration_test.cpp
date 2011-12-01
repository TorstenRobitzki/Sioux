// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "bayeux/configuration.h"

BOOST_AUTO_TEST_CASE( max_disconnected_time_config_test )
{
	bayeux::configuration config;

	BOOST_CHECK_EQUAL( &config.max_disconnected_time( boost::posix_time::seconds( 42 ) ), &config );
	BOOST_CHECK_EQUAL( config.max_disconnected_time(), boost::posix_time::seconds( 42 ) );

	BOOST_CHECK_EQUAL( &config.max_disconnected_time( boost::posix_time::seconds( 2 ) ), &config );
	BOOST_CHECK_EQUAL( config.max_disconnected_time(), boost::posix_time::seconds( 2 ) );

	BOOST_CHECK_EQUAL( &config.max_disconnected_time( boost::posix_time::seconds( 1 ) ), &config );
	BOOST_CHECK_EQUAL( config.max_disconnected_time(), boost::posix_time::seconds( 1 ) );
}

BOOST_AUTO_TEST_CASE( max_messages_per_client_config_test )
{
	bayeux::configuration config;

	BOOST_CHECK_EQUAL( &config.max_messages_per_client( 22u ), &config );
	BOOST_CHECK_EQUAL( config.max_messages_per_client(), 22u );

	BOOST_CHECK_EQUAL( &config.max_messages_per_client( 0 ), &config );
	BOOST_CHECK_EQUAL( config.max_messages_per_client(), 0 );

	BOOST_CHECK_EQUAL( &config.max_messages_per_client( 444u ), &config );
	BOOST_CHECK_EQUAL( config.max_messages_per_client(), 444u );
}

BOOST_AUTO_TEST_CASE( max_messages_size_per_client_config_test )
{
	bayeux::configuration config;

	BOOST_CHECK_EQUAL( &config.max_messages_size_per_client( 22u ), &config );
	BOOST_CHECK_EQUAL( config.max_messages_size_per_client(), 22u );

	BOOST_CHECK_EQUAL( &config.max_messages_size_per_client( 0 ), &config );
	BOOST_CHECK_EQUAL( config.max_messages_size_per_client(), 0 );

	BOOST_CHECK_EQUAL( &config.max_messages_size_per_client( 102400u ), &config );
	BOOST_CHECK_EQUAL( config.max_messages_size_per_client(), 102400u );
}

BOOST_AUTO_TEST_CASE( check_configuration_items_are_independent )
{
	bayeux::configuration config;
	config.max_disconnected_time( boost::posix_time::minutes(1) )
		.max_messages_per_client( 4u )
		.max_messages_size_per_client( 1234u );

	BOOST_CHECK_EQUAL( config.max_disconnected_time(), boost::posix_time::minutes(1) );
	BOOST_CHECK_EQUAL( config.max_messages_per_client(), 4u );
	BOOST_CHECK_EQUAL( config.max_messages_size_per_client(), 1234u );
}
