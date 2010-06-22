// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/unittest++.h"
#include "server/connection.h"
#include "server/test_traits.h"
#include "server/test_response.h"
#include "http/test_request_texts.h"
#include "http/request.h"
#include "http/response.h"
#include "tools/iterators.h"
#include <boost/asio/buffer.hpp>

using namespace server::test;
using namespace http::test;

static void simulate_incomming_data(const boost::weak_ptr<server::async_response>& resp)
{
    typedef server::test::response<server::connection<traits<> > > response;

    boost::static_pointer_cast<response>(boost::shared_ptr<server::async_response>(resp))->simulate_incomming_data();
}

/** 
 * @test test that responses go in the right order onto the wire.
 *
 * For a number or responses there is simulated, that they start to send there data in different order
 */ 
TEST(simply_receiving_a_hello)
{

    traits<>::connection_type   socket(begin(simple_get_11), begin(simple_get_11), 5);
    traits<>                    trait;

    boost::shared_ptr<server::connection<traits<> > > connection = server::create_connection(socket, trait);

    typedef server::test::response<server::connection<traits<> > > response;

    // a simulated response to an empty header
    boost::shared_ptr<http::request_header> empty_header(new http::request_header);

    const char* texts[] = {"Hallo,", " wie ", "gehts?"};
    int         index[] = {0, 1, 2};

    do
    {
        typedef std::vector<boost::weak_ptr<server::async_response> > response_list_t;
        response_list_t responses;

        for ( const char* const* t = tools::begin(texts); t != tools::end(texts); ++t )
            responses.push_back(create_response(connection, empty_header, *t, manuel_response));

        // then we simulate that the IO is fullfied
        for ( int* sim = tools::begin(index); sim != tools::end(index); ++sim )
        {
            boost::static_pointer_cast<response>(boost::shared_ptr<server::async_response>(responses[*sim]))->simulate_incomming_data();

            // now every response that was created before, must have been asked to hurry
            for ( int r = *sim; r >0; --r )
            {
                const boost::weak_ptr<server::async_response> resp = responses[r-1];

                if ( !resp.expired() )
                {
                    CHECK(boost::shared_ptr<server::async_response>(resp)->asked_to_hurry());
                }
            }
        }

        while ( socket.process() )
            ;

        // all responses must have been destroyed
        for ( response_list_t::const_iterator resp = responses.begin(), end = responses.end(); resp != end; ++resp )
        {
            CHECK_EQUAL(0, resp->use_count());
        }

        // and finaly, the ouput must be same for every order of response
        CHECK_EQUAL("Hallo, wie gehts?", socket.output());
    } while ( std::next_permutation(tools::begin(index), tools::end(index)) );

}

/** 
 * @test this test should ensure, that when a request in the middle of the pipeline reports an error 
 *       other reponses will fulfill.
 */
TEST(non_fatal_error_while_responding)
{
    traits<>::connection_type   socket(begin(simple_get_11), begin(simple_get_11));
    traits<>                    trait;

    boost::shared_ptr<server::connection<traits<> > >   connection = server::create_connection(socket, trait);
    boost::shared_ptr<http::request_header>             empty_header(new http::request_header);

    boost::weak_ptr<server::async_response> first_response = 
        create_response(connection, empty_header, "http/1.1 100 go ahead\r\n\r\n", manuel_response);

    boost::weak_ptr<server::async_response> second_response = 
        create_response(connection, empty_header, http::http_not_found, manuel_response);

    boost::weak_ptr<server::async_response> third_response = 
        create_response(connection, empty_header, "http/1.1 101 go ahead\r\n\r\n", manuel_response);

    simulate_incomming_data(third_response);
    simulate_incomming_data(second_response);
    CHECK(boost::shared_ptr<server::async_response>(first_response)->asked_to_hurry());
    simulate_incomming_data(first_response);

    while ( socket.process() )
        ;

    std::size_t             size = socket.output().size();
    http::response_header   first(socket.output().c_str());
    http::response_header   second(first, size, http::response_header::copy_trailing_buffer);
    http::response_header   third(first, size, http::response_header::copy_trailing_buffer);

    CHECK_EQUAL(std::size_t(0), size);
    CHECK_EQUAL(http::response_header::ok, first.state());
    CHECK_EQUAL(http::response_header::ok, second.state());
    CHECK_EQUAL(http::response_header::ok, third.state());
    CHECK_EQUAL(http::http_continue, first.code());
    CHECK_EQUAL(http::http_not_found, first.code());
    CHECK_EQUAL(http::http_switching_protocols, first.code());

    CHECK(first_response.expired());
    CHECK(second_response.expired());
    CHECK(third_response.expired());

    boost::weak_ptr<server::connection<traits<> > > weak_con = connection;
    connection.reset();
    CHECK(weak_con.expired());
}

/** 
 * @test this test should ensure, that when a request in the middle of the pipeline reports a fatail error 
 *       other reponses will be canceled.
 */
TEST(fatal_error_while_responding)
{

}
