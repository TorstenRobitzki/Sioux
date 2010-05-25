// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/unittest++.h"
#include "server/connection.h"
#include "server/test_traits.h"
#include "server/test_response.h"
#include "server/test_request_texts.h"
#include "server/request.h"
#include "tools/iterators.h"
#include <boost/asio/buffer.hpp>

/** 
 * @test test that responses go in the right order onto the wire.
 *
 * For a number or responses there is simulated, that they start to send there data in different order
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

