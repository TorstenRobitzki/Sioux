// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#define BOOST_TEST_MAIN

#include "file/response.h"
#include "server/test_traits.h"
#include "server/connection.h"
#include "tools/iterators.h"
#include "tools/io_service.h"
#include "http/decode_stream.h"
#include "http/response.h"
#include "http/test_tools.h"
#include "asio_mocks/test_timer.h"

#include <boost/test/unit_test.hpp>
#include <iterator>
#include <fstream>
#include <iostream>

namespace
{
    struct response_factory
    {
        response_factory() {}

        template < class T >
        explicit response_factory( const T& ) {}

        template < class Connection >
        boost::shared_ptr< server::async_response > create_response(
            const boost::shared_ptr< Connection >&                    connection,
            const boost::shared_ptr< const http::request_header >&    header )
        {
            const tools::substring  uri = header->uri();
            const boost::shared_ptr< server::async_response > new_response(
                new file::response< Connection >( connection, boost::filesystem::path( uri.begin(), uri.end() ) ) );
            return new_response;
        }

        template <class Connection>
        boost::shared_ptr< server::async_response > error_response(
            const boost::shared_ptr<Connection>& con, http::http_error_code ec ) const
        {
            boost::shared_ptr< server::async_response > result( new ::server::error_response< Connection >( con, ec ) );
            return result;
        }
    };

    typedef asio_mocks::socket< const char* > socket_t;

    typedef server::connection_traits<
        socket_t,
        asio_mocks::timer,
        response_factory,
        server::null_event_logger,
        server::stream_error_log > trait_t;

    typedef server::connection< trait_t > connection_t;
}

static const char get_this_file[] =
    "GET "__FILE__" HTTP/1.1\r\n"
    "Host: google.de\r\n"
    "User-Agent: Web-sniffer/1.0.31 (+http://web-sniffer.net/)\r\n"
    "Accept-Encoding: gzip\r\n"
    "Accept-Charset: ISO-8859-1,UTF-8;q=0.7,*;q=0.7\r\n"
    "Cache-Control: no\r\n"
    "Accept-Language: de,en;q=0.7,en-us;q=0.3\r\n"
    "Referer: http://web-sniffer.net/\r\n"
    "\r\n";

static bool equal_to_this_file( const std::vector<char>& read )
{
    std::ifstream input( __FILE__ );
    std::istreambuf_iterator< char > begin( input ), end;
    const std::vector< char > self( begin, end );

    return http::test::compare_buffers( read, self, std::cerr );
}

BOOST_AUTO_TEST_CASE( retrieve_an_existing_file )
{
    boost::asio::io_service queue;
    socket_t                socket( queue, tools::begin( get_this_file ), tools::end( get_this_file ) -1 );
    trait_t                 trait;

    boost::shared_ptr< connection_t > connection( new connection_t( socket, trait ) );
    connection->start();

    tools::run( queue );

    std::vector< std::pair< boost::shared_ptr< http::response_header >, std::vector< char > > > response =
        http::decode_stream< http::response_header >( socket.bin_output() );

    BOOST_REQUIRE_EQUAL( response.size(), 1u );

    BOOST_CHECK_EQUAL( response.front().first->code(), http::http_ok );
    BOOST_CHECK( equal_to_this_file( response.front().second ) );
}

BOOST_AUTO_TEST_CASE( retrieve_a_not_existing_file )
{
    static const char get_fantasy_file[] =
        "GET /lottozahlen/morgen HTTP/1.1\r\n"
        "Host: google.de\r\n"
        "User-Agent: Web-sniffer/1.0.31 (+http://web-sniffer.net/)\r\n"
        "Accept-Encoding: gzip\r\n"
        "Accept-Charset: ISO-8859-1,UTF-8;q=0.7,*;q=0.7\r\n"
        "Cache-Control: no\r\n"
        "Accept-Language: de,en;q=0.7,en-us;q=0.3\r\n"
        "Referer: http://web-sniffer.net/\r\n"
        "\r\n";

    boost::asio::io_service queue;
    socket_t                socket( queue, tools::begin( get_fantasy_file ), tools::end( get_fantasy_file ) -1 );
    trait_t                 trait;

    boost::shared_ptr< connection_t > connection( new connection_t( socket, trait ) );
    connection->start();

    tools::run( queue );

    std::vector< std::pair< boost::shared_ptr< http::response_header >, std::vector< char > > > response =
        http::decode_stream< http::response_header >( socket.bin_output() );

    BOOST_REQUIRE_EQUAL( response.size(), 1u );
    BOOST_CHECK_EQUAL( response.front().first->code(), http::http_not_found );
}
