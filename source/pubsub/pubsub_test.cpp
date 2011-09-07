// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include <boost/test/unit_test.hpp>
#include <boost/bind.hpp>

#include "pubsub/stress_adapter.h"
#include "pubsub/stress_subscriber.h"
#include "tools/io_service.h"

/*!
 * @test different users with different actions on the very same root
 *
 * The purpose of this test is to see, that no ugly race conditions will result
 * in double locks, dead locks or heap corruptions.
 *
 * @todo this test isn't implemented atm
 */
BOOST_AUTO_TEST_CASE( pubsub_stress_test )
{
	/*
	using namespace pubsub;

	boost::asio::io_service	queue;
	test::stress_adapter	test_adapter( queue );
	root 					nodes(queue, test_adapter, pubsub::configuration());

	static const unsigned number_of_users 				= 5u;
	static const unsigned number_of_requests_per_user 	= 1000u;

	std::vector< boost::shared_ptr< test::stress_subscriber > > users;
	for ( unsigned user_cnt = 0; user_cnt != number_of_users; ++user_cnt )
	{
		boost::shared_ptr<test::stress_subscriber> new_user(
				new test::stress_subscriber(nodes, test_adapter, number_of_requests_per_user, user_cnt ) );
		new_user->start();
		users.push_back( new_user );
	}

	tools::run( queue );

	std::for_each( users.begin(), users.end(), boost::bind( &test::stress_subscriber::check, _1 ) );
	*/
}

