// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/unittest++.h"
#include "server/connection.h"
#include "server/test_traits.h"
#include "server/test_response.h"
#include "server/test_request_texts.h"
#include "server/request.h"
#include <boost/asio/buffer.hpp>

/** 
 * @test test that the 
 */ 
TEST(simply_receiving_a_hello)
{
    using namespace server::test;

    traits<>::connection_type   socket(begin(simple_get_11), begin(simple_get_11), 5);
    traits<>                    trait;

    boost::shared_ptr<server::connection<traits<> > > connection = server::create_connection(socket, trait);

    typedef server::test::response<server::connection<traits<> > > response;

    // a simulated response to an empty header
    boost::shared_ptr<server::request_header>       empty_header(new server::request_header);

    boost::weak_ptr<server::async_response> response1 = create_response(connection, empty_header, "hello");
    boost::weak_ptr<server::async_response> response2 = create_response(connection, empty_header, " wie");
    boost::weak_ptr<server::async_response> response3 = create_response(connection, empty_header, " gehts?");

    // then we simulate that the IO is fullfied
    boost::static_pointer_cast<response>(boost::shared_ptr<server::async_response>(response1))->simulate_incomming_data();
    boost::static_pointer_cast<response>(boost::shared_ptr<server::async_response>(response2))->simulate_incomming_data();
    boost::static_pointer_cast<response>(boost::shared_ptr<server::async_response>(response3))->simulate_incomming_data();

    while ( socket.process() )
        ;

    CHECK_EQUAL(0, response1.use_count());
    CHECK_EQUAL(0, response2.use_count());
    CHECK_EQUAL(0, response3.use_count());

    CHECK_EQUAL("hello wie gehts?", socket.output());

}

