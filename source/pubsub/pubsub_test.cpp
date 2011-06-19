// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include <boost/test/unit_test.hpp>

#include "pubsub/stress_subscriber.h"
#include "pubsub/stress_adapter.h"

/*!
 * @test different users with different actions on the very same root
 */
BOOST_AUTO_TEST_CASE(pubsub_stress_test)
{
	/*
	using namespace pubsub;

	test::stress_adapter	test_adapter;
	boost::asio::io_service	queue;
	root 					nodes(queue, test_adapter, pubsub::configuration());

	static const unsigned number_of_users 				= 5u;
	static const unsigned number_of_requests_per_user 	= 1000u;

	for ( unsigned user_cnt = 0; user_cnt != number_of_users; ++user_cnt )
	{
		boost::shared_ptr<test::stress_subscriber> new_user( new test::stress_subscriber(nodes, number_of_requests_per_user) );
		new_user->start();
	}*/
}
