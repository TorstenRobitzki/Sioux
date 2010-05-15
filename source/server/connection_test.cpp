// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/unittest++.h"
#include "server/connection.h"
#include "server/test_traits.h"
#include "server/test_request_texts.h"

TEST(read_simple_header)
{
    using namespace server::test;

    traits<>::connection_type   socket(begin(simple_get_11), end(simple_get_11), 5);
    traits<>                    traits;

    server::create_connection(socket, traits);

    for (; socket.process(); )
        ;

    CHECK_EQUAL(1, traits.requests().size());
}

TEST(read_multiple_header)
{
    using namespace server::test;

    traits<>::connection_type   socket(begin(simple_get_11), end(simple_get_11), 400, 2000);
    traits<>                    traits;

    server::create_connection(socket, traits);

    for (; socket.process(); )
        ;

    CHECK_EQUAL(2000, traits.requests().size());
}

TEST(read_big_buffer)
{
    using namespace server::test;

    std::vector<char>   input;
    for ( unsigned i = 0; i != 1000; ++i )
        input.insert(input.end(), begin(simple_get_11), end(simple_get_11));

    typedef traits<server::test::socket<std::vector<char>::const_iterator> > socket_t;
    socket_t::connection_type   socket(input.begin(), input.end());
    traits<>                    traits;

    server::create_connection(socket, traits);

    for (; socket.process(); )
        ;

    CHECK_EQUAL(1000, traits.requests().size());
}

TEST(read_buffer_overflow)
{
    using namespace server::test;

    const char header[] = "Accept-Encoding: gzip\r\n";

    std::vector<char>   input(begin(request_without_end_line), end(request_without_end_line));
    for ( unsigned i = 0; i != 10000; ++i )
        input.insert(input.end(), begin(header), end(header));

    typedef traits<server::test::socket<std::vector<char>::const_iterator> > socket_t;
    socket_t::connection_type   socket(input.begin(), input.end());
    traits<>                    traits;

    server::create_connection(socket, traits);

    for (; socket.process(); )
        ;

    CHECK_EQUAL(1, traits.requests().size());
    CHECK_EQUAL(server::request_header::buffer_full, traits.requests().front()->state());
}

