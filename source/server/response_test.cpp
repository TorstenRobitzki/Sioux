// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include <boost/test/unit_test.hpp>
#include "server/connection.h"
#include "server/test_traits.h"
#include "server/test_response.h"
#include "server/test_tools.h"
#include "http/test_request_texts.h"
#include "http/request.h"
#include "http/response.h"
#include "tools/iterators.h"
#include "tools/io_service.h"
#include <boost/asio/buffer.hpp>

using namespace server::test;
using namespace http::test;

namespace {

    void simulate_incomming_data(const boost::weak_ptr<server::async_response>& resp)
    {
        typedef server::test::response<server::connection<traits<> > > response;

        boost::static_pointer_cast<response>(boost::shared_ptr<server::async_response>(resp))->simulate_incomming_data();
    }

    struct hello_world_response_factory
    {
        static const char*  texts[3];
        static const char** next_response;

        template < class Trait, class Connection >
        static boost::shared_ptr<server::async_response> create_response(
            const boost::shared_ptr<Connection>&                    connection,
            const boost::shared_ptr<const http::request_header>&    header,
                  Trait&)
        {
            const boost::shared_ptr<response<Connection> > new_response(
                new response<Connection>(connection, header, *next_response, manuel_response));

            ++next_response;

            if ( next_response == tools::end(texts) )
                next_response = tools::begin(texts);

            return boost::shared_ptr<server::async_response>(new_response);
        }
    };

    const char*  hello_world_response_factory::texts[3] = {"Hallo,", " wie ", "gehts?"};
    const char** hello_world_response_factory::next_response = tools::begin(texts);

}

/** 
 * @test test that responses go in the right order onto the wire.
 *
 * For a number or responses there is simulated, that they start to send there data in different order
 */ 
BOOST_AUTO_TEST_CASE(simply_receiving_a_hello)
{

    typedef server::test::socket<const char*>                       socket_t;
    typedef traits< hello_world_response_factory >                    trait_t;
    typedef server::connection<trait_t>                             connection_t;
    typedef std::vector<boost::weak_ptr<server::async_response> >   response_list_t;

    // the order in which the responses get ready 
    int index[] = { 0, 1, 2 };

    do { 
        boost::asio::io_service         queue;
        socket_t                        socket(queue, begin(simple_get_11), end(simple_get_11), 0, 3);
        trait_t                         trait;
        boost::shared_ptr<connection_t> connection = server::create_connection(socket, trait);

        tools::run(queue);

        std::vector<boost::shared_ptr<server::async_response> > resp = trait.responses();
        BOOST_CHECK_EQUAL(3u, resp.size());
        
        const response_list_t responses(resp.begin(), resp.end());
        resp.clear();
        
        // then we simulate that the IO is fullfied
        for ( const int* i = tools::begin(index); i != tools::end(index); ++i )
        {
            simulate_incomming_data(responses[*i]);

            // now every response that was created before, must have been asked to hurry
            for ( int r = *i; r >0; --r )
            {
                BOOST_CHECK(responses[r-1].expired()
                    || boost::shared_ptr<server::async_response>(responses[r-1])->asked_to_hurry());
            }
        }

        tools::run(queue);

        trait.reset_responses();

        // all responses must have been destroyed
        for ( response_list_t::const_iterator resp = responses.begin(), end = responses.end(); resp != end; ++resp )
        {
            BOOST_CHECK_EQUAL(0, resp->use_count());
        }

        // and finaly, the ouput must be the same for every order of response
        BOOST_CHECK_EQUAL("Hallo, wie gehts?", socket.output());
    } while ( std::next_permutation(tools::begin(index), tools::end(index)) );

}

namespace {

    struct error_response_factory
    {
        static int context;

        template < class Trait, class Connection >
        static boost::shared_ptr<server::async_response> create_response(
            const boost::shared_ptr<Connection>&                    connection,
            const boost::shared_ptr<const http::request_header>&    header,
                  Trait&)
        {
            switch ( context++ % 3 )
            {
            case 0:
                return  boost::shared_ptr<server::async_response>(
                    new response<Connection>(connection, header, "http/1.1 100 go ahead\r\n\r\n", manuel_response));
                break;
            case 1:
                return  boost::shared_ptr<server::async_response>(
                    new response<Connection>(connection, header, http::http_not_found, manuel_response));
                break;
            case 2:
                return  boost::shared_ptr<server::async_response>(
                    new response<Connection>(connection, header, "http/1.1 101 go ahead\r\n\r\n", manuel_response));
                break;
            }

            // to supress compiler warnings
            return boost::shared_ptr<server::async_response>();
        }
    };

    int error_response_factory::context = 0;
}

/** 
 * @test this test should ensure, that when a request in the middle of the pipeline reports an error 
 *       other reponses will fulfill.
 */
BOOST_AUTO_TEST_CASE(non_fatal_error_while_responding)
{
    typedef server::test::socket<>              socket_t;
    typedef traits< error_response_factory >    trait_t;
    typedef server::connection< trait_t >       connection_t;

    boost::asio::io_service queue;
    socket_t                socket(queue, begin(simple_get_11), end(simple_get_11), 0, 3);
    trait_t                 trait;

    boost::shared_ptr<connection_t> connection = server::create_connection(socket, trait);

    tools::run(queue);

    std::vector<boost::shared_ptr<server::async_response> > resp = trait.responses();
    trait.reset_responses();

    simulate_incomming_data(resp[2]);
    simulate_incomming_data(resp[1]);
    BOOST_CHECK(boost::shared_ptr<server::async_response>(resp[0])->asked_to_hurry());
    simulate_incomming_data(resp[0]);

    tools::run(queue);

    const std::string       output = socket.output();
    std::size_t             size = output.size();
    http::response_header   first(output.c_str());
    http::response_header   second(first, size, http::response_header::copy_trailing_buffer);
    assert(size);
    second.parse(size);
    http::response_header   third(second, size, http::response_header::copy_trailing_buffer);
    assert(size);
    third.parse(size);

    BOOST_CHECK_EQUAL( http::response_header::ok, first.state() );
    BOOST_CHECK_EQUAL( http::response_header::ok, second.state() );
    BOOST_CHECK_EQUAL( http::response_header::ok, third.state() );
    BOOST_CHECK_EQUAL( http::http_continue, first.code() );
    BOOST_CHECK_EQUAL( http::http_not_found, second.code() );
    BOOST_CHECK_EQUAL( http::http_switching_protocols, third.code() );

    BOOST_CHECK_EQUAL( 1, resp[0].use_count() );
    BOOST_CHECK_EQUAL( 1, resp[1].use_count() );
    BOOST_CHECK_EQUAL( 1, resp[2].use_count() );

    resp.clear();
    BOOST_CHECK_EQUAL( 1, connection.use_count() );
}

/** 
 * @test this test should ensure, that when a request in the middle of the pipeline reports a fatal error
 *       other responses will be canceled.
 */
BOOST_AUTO_TEST_CASE(fatal_error_while_responding)
{
    /// @todo implement
}   
