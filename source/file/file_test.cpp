// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "file/file.h"
#include "server/error.h"
#include "server/test_socket.h"
#include "server/test_timer.h"
#include "server/log.h"
#include "server/traits.h"
#include "server/connection.h"
#include "tools/io_service.h"
#include "http/decode_stream.h"
#include "http/response.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE( file_root_in_not_existing_directory )
{
    BOOST_CHECK_THROW(
        file::file_root( "/etc/ffoo/bbar/cchu" ), std::runtime_error );
}

BOOST_AUTO_TEST_CASE( file_root_is_no_a_directory )
{
    BOOST_CHECK_THROW(
        file::file_root( __FILE__ ), std::runtime_error );
}

namespace
{
    std::string construct_request( const char* uri )
    {
        const std::string result =
            std::string( "GET " ) + uri
                +   " HTTP/1.1\r\n"
                    "Host: google.de\r\n"
                    "User-Agent: Web-sniffer/1.0.31 (+http://web-sniffer.net/)\r\n"
                    "Accept-Encoding: gzip\r\n"
                    "Accept-Charset: ISO-8859-1,UTF-8;q=0.7,*;q=0.7\r\n"
                    "Cache-Control: no\r\n"
                    "Accept-Language: de,en;q=0.7,en-us;q=0.3\r\n"
                    "Referer: http://web-sniffer.net/\r\n"
                    "\r\n";

        return result;
    }

    struct response_factory
    {
        template < class T >
        explicit response_factory( const T& root )
            : root_( root.root )
        {
        }

        template < class Connection >
        boost::shared_ptr< server::async_response > create_response(
            const boost::shared_ptr< Connection >&                    connection,
            const boost::shared_ptr< const http::request_header >&    header )
        {
            return root_.create_response( connection, header );
        }

        template <class Connection>
        boost::shared_ptr< server::async_response > error_response(
            const boost::shared_ptr<Connection>& con, http::http_error_code ec ) const
        {
            boost::shared_ptr< server::async_response > result( new ::server::error_response< Connection >( con, ec ) );
            return result;
        }

        file::file_root root_;
    };

    typedef server::test::socket< const char* > socket_t;

    typedef server::connection_traits<
        socket_t,
        server::test::timer,
        response_factory,
        server::null_event_logger,
        server::stream_error_log > trait_t;

    typedef server::connection< trait_t > connection_t;

    struct context
    {
        boost::filesystem::path root;

        std::ostream& logstream() const { return std::cout; }
    };

    std::pair< http::http_error_code, std::string > get_file( const boost::filesystem::path& root, const char* uri )
    {
        boost::asio::io_service queue;
        const std::string       request = construct_request( uri );
        socket_t                socket( queue, request.c_str(), request.c_str() + request.size() );

        context                 trait_params = { root };
        trait_t                 trait( trait_params );

        boost::shared_ptr< connection_t > connection( new connection_t( socket, trait ) );
        connection->start();

        tools::run( queue );

        http::decoded_response_stream_t result = http::decode_stream< http::response_header >( socket.bin_output() );

        BOOST_REQUIRE_EQUAL( result.size(), 1u );

        const std::pair< boost::shared_ptr< http::response_header >, std::vector< char > >& element = result.front();

        return std::make_pair( element.first->code(), std::string( element.second.begin(), element.second.end() ) );
    }

    boost::filesystem::path test_root()
    {
        return boost::filesystem::canonical( __FILE__ ).parent_path() / "root";
    }
}

BOOST_AUTO_TEST_CASE( accessing_file_from_sub_root )
{
    const std::pair< http::http_error_code, std::string > result = get_file( test_root(), "../file_test.cpp" );

    BOOST_CHECK_EQUAL( result.first, http::http_forbidden );
    BOOST_CHECK( result.second.empty() );
}

BOOST_AUTO_TEST_CASE( accessing_file_from_root )
{
    const std::pair< http::http_error_code, std::string > root_result = get_file( test_root(), "/root.txt" );
    BOOST_CHECK_EQUAL( root_result.first, http::http_ok );
    BOOST_CHECK_EQUAL( root_result.second, "root_text" );
}

BOOST_AUTO_TEST_CASE( accessing_file_below_root )
{
    const std::pair< http::http_error_code, std::string > a_result = get_file( test_root(), "/a/1.txt" );
    BOOST_CHECK_EQUAL( a_result.first, http::http_ok );
    BOOST_CHECK_EQUAL( a_result.second, "a" );

    const std::pair< http::http_error_code, std::string > b_result = get_file( test_root(), "/b/1.txt" );
    BOOST_CHECK_EQUAL( b_result.first, http::http_ok );
    BOOST_CHECK_EQUAL( b_result.second, "b" );
}
