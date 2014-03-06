#include <boost/test/unit_test.hpp>
#include "pubsub/stress_adapter.h"
#include "pubsub/test_helper.h"
#include "pubsub/root.h"
#include "pubsub/configuration.h"
#include "json/json.h"
#include "tools/io_service.h"

using namespace pubsub;

namespace
{
	node_name create_node_name(const std::string& json_text)
	{
		return node_name(json::parse(json_text).upcast<json::object>());
	}

	test::subscriber& test_user(const boost::shared_ptr< ::pubsub::subscriber>& u)
	{
		assert(u.get());
		return dynamic_cast<test::subscriber&>(*u.get());
	}
}

/*!
 * \test test that the "valid" domain is used to decide the validity of a subscribed node
 *       and that the "init" domains value is used a initial node value.
 */
BOOST_AUTO_TEST_CASE( stress_adapter_sync_validation )
{
	boost::asio::io_service	queue;
	test::stress_adapter	test_object(queue);
	const configuration     test_config = configurator().authorization_not_required();
	root					nodes(queue, test_object, test_config);

	const node_name         valid_name   = create_node_name("{\"a\":\"1\", \"valid\":\"valid\", \"init\":\"42\" }");
	const node_name         invalid_name = create_node_name("{\"a\":\"1\", \"valid\":\"nope\", \"init\":\"42\" }");

	boost::shared_ptr<subscriber> user( new test::subscriber );

	nodes.subscribe(user, valid_name);
	BOOST_CHECK(test_user(user).not_on_failed_node_subscription_called());
	BOOST_CHECK(test_user(user).not_on_unauthorized_node_subscription_called());
	BOOST_CHECK(test_user(user).not_on_invalid_node_subscription_called());

	tools::run(queue);

	BOOST_CHECK(test_user(user).on_update_called(valid_name, json::number(42)));
	BOOST_CHECK(test_user(user).empty());

	nodes.subscribe(user, invalid_name);
	BOOST_CHECK(test_user(user).on_invalid_node_subscription_called(invalid_name));

	tools::run(queue);
	BOOST_CHECK(test_user(user).empty());
}

/*!
 * \test test that the validation can be configured to be asynchronous, both for valid and invalid node names.
 */
BOOST_AUTO_TEST_CASE( stress_adapter_async_validation )
{
	boost::asio::io_service	queue;
	test::stress_adapter	test_object(queue);
	const configuration     test_config = configurator().authorization_not_required();
	root					nodes(queue, test_object, test_config);

	const node_name         valid_name   = create_node_name("{\"a\":\"1\", \"valid\":\"async_valid\", \"init\":42 }");
	const node_name         invalid_name = create_node_name("{\"a\":\"1\", \"valid\":\"async_invalid\", \"init\":42 }");

	boost::shared_ptr<subscriber> user( new test::subscriber );

	nodes.subscribe(user, valid_name);
	BOOST_CHECK(test_user(user).not_on_failed_node_subscription_called());
	BOOST_CHECK(test_user(user).not_on_unauthorized_node_subscription_called());
	BOOST_CHECK(test_user(user).not_on_invalid_node_subscription_called());
	BOOST_CHECK(test_user(user).not_on_update_called());

	tools::run(queue);

	BOOST_CHECK(test_user(user).on_update_called(valid_name, json::number(42)));
	BOOST_CHECK(test_user(user).empty());

	nodes.subscribe(user, invalid_name);
	BOOST_CHECK(test_user(user).not_on_invalid_node_subscription_called());

	tools::run(queue);
	BOOST_CHECK(test_user(user).on_invalid_node_subscription_called(invalid_name));
	BOOST_CHECK(test_user(user).empty());
}

/*!
 * \test verify that node initialization can be synchronous and asynchronous initialized
 */
BOOST_AUTO_TEST_CASE(stress_adapter_initialization)
{
	const node_name         synchronous   = create_node_name("{\"valid\":\"valid\", \"init\" : 42 }");
	const node_name         asynchronous  = create_node_name("{\"valid\":\"valid\", \"async_init\" : 43 }");
	const node_name         invalid       = create_node_name("{\"valid\":\"valid\" }");
	const node_name         async_invalid = create_node_name("{\"valid\":\"valid\", \"async_init_fail\":1 }");

	boost::asio::io_service	queue;
	test::stress_adapter	test_object(queue);
	const configuration     test_config = configurator().authorization_not_required();
	root					nodes(queue, test_object, test_config);

	boost::shared_ptr<subscriber> user( new test::subscriber );

	nodes.subscribe(user, synchronous);
	const std::size_t calls_till_sync_init = tools::run(queue);
	BOOST_CHECK(test_user(user).on_update_called(synchronous, json::number(42)));

	nodes.subscribe(user, asynchronous);
	const std::size_t calls_till_async_init = tools::run(queue);
	BOOST_CHECK(test_user(user).on_update_called(asynchronous, json::number(43)));

	// the asynchronous initialization must last at least one call more
	BOOST_CHECK(calls_till_async_init > calls_till_sync_init);

	nodes.subscribe(user, invalid);
	const std::size_t calls_till_failure = tools::run(queue);
	BOOST_CHECK(test_user(user).on_failed_node_subscription_called(invalid));

	nodes.subscribe(user, async_invalid);
	const std::size_t calls_till_async_failure = tools::run(queue);
	BOOST_CHECK(test_user(user).on_failed_node_subscription_called(async_invalid));

	BOOST_CHECK(calls_till_async_failure > calls_till_failure);
}

/*!
 * \test check, that authorization depends on the internal per subscriber lists and on the asynchronous configuration
 */
BOOST_AUTO_TEST_CASE(stress_adapter_authorisation)
{
	boost::asio::io_service	queue;
	test::stress_adapter	test_object(queue);
	const configuration     test_config = configurator().authorization_required();
	root					nodes(queue, test_object, test_config);

	const node_name         authorised       = create_node_name("{ \"valid\" : \"valid\", \"init\" : 42 }");
	const node_name         async_authorised = create_node_name("{ \"valid\" : \"valid\", \"init\" : 44, \"async_auth\" : 1 }");

	boost::shared_ptr<subscriber> user( new test::subscriber );
	boost::shared_ptr<subscriber> evil_user( new test::subscriber );

	test_object.add_authorization(*user, authorised);
	test_object.add_authorization(*user, async_authorised);

	nodes.subscribe(user, authorised);
	const std::size_t calls_till_sync_init = tools::run(queue);
	BOOST_CHECK(test_user(user).on_update_called(authorised, json::number(42)));

	nodes.subscribe(user, async_authorised);
	const std::size_t calls_till_async_init = tools::run(queue);
	BOOST_CHECK(test_user(user).on_update_called(async_authorised, json::number(44)));

	BOOST_CHECK( calls_till_sync_init < calls_till_async_init );

	// unsubscribe and not being authorized anymore
	nodes.unsubscribe(user, authorised);
	test_object.remove_authorization(*user, authorised);

	nodes.subscribe(user, authorised);
	tools::run(queue);
	BOOST_CHECK(test_user(user).on_unauthorized_node_subscription_called(authorised));
}

