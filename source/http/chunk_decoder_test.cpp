#include <boost/test/unit_test.hpp>
#include "http/chunk_decoder.h"
#include "http/test_tools.h"
#include "http/test_request_texts.h"
#include <vector>
#include <iostream>
#include <algorithm>

namespace {
    class body_receiver : public http::chunk_decoder< body_receiver >
    {
    public:
        explicit body_receiver( std::size_t max_accept ) : max_accept_( max_accept )
        {
        }

        std::size_t take_chunk( const char* input, std::size_t size )
        {
            const std::size_t take = std::min( size, max_accept_ );
            body_.insert( body_.end(), input, input + take );

            return take;
        }

        const std::vector< char > body() const
        {
            return body_;
        }

        std::string body_str() const
        {
            return std::string( body_.begin(), body_.end() );
        }
    private:
        const std::size_t   max_accept_;
        std::vector< char > body_;
    };

    template < std::size_t DecodeSize, std::size_t BodySize >
    std::size_t test_body_decode( const char ( &decoded )[ DecodeSize ], const char ( &body )[ BodySize ], std::size_t max_accept, std::size_t max_feed )
    {
        body_receiver decoder( max_accept );

        const char*       begin = &decoded[ 0 ];
        const char* const end   = &decoded[ DecodeSize - 1 ];

        while ( begin != end && !decoder.chunked_done() )
        {
            const std::size_t feed_size = std::min( static_cast< std::size_t >( end - begin ), max_feed );
            const std::size_t current_feed = decoder.feed_chunked_buffer( begin, feed_size );

            BOOST_CHECK_GT( current_feed, 0 );
            BOOST_CHECK_LE( current_feed, feed_size );

            begin += current_feed;
        }

        BOOST_CHECK( http::test::compare_buffers( std::vector< char >( body, body + BodySize - 1 ), decoder.body(), std::cerr ) );
        BOOST_CHECK( decoder.chunked_done() );

        return end - begin;
    }

    template < std::size_t DecodeSize, std::size_t BodySize >
    std::size_t test_body_decode_variations( const char ( &decoded )[ DecodeSize ], const char ( &body )[ BodySize ] )
    {
        typedef std::pair< std::size_t, std::size_t > pair_t;
        const pair_t sizes[] = { pair_t( 1, 1 ), pair_t( 1, 10 ), pair_t( 10, 1 ), pair_t( 50, 30 ) };

        std::vector< std::size_t > results;
        for ( int i = 0; i != sizeof( sizes ) / sizeof( sizes[ 0 ] ); ++i )
            results.push_back( test_body_decode( decoded, body, sizes[ i ].first, sizes[ i ].second ) );

        for ( std::vector< std::size_t >::const_iterator not_same_rest = results.begin(); not_same_rest != results.end(); ++not_same_rest )
            BOOST_CHECK_EQUAL( *not_same_rest, results.front() );

        return results.front();
    }
}


BOOST_AUTO_TEST_CASE( feeding_an_erroneous_size_will_result_in_exception )
{
    body_receiver decoder( 100 );
    const char not_a_size[] = "axc\r\n";
    BOOST_CHECK_THROW( decoder.feed_chunked_buffer( &not_a_size[ 0 ], sizeof( not_a_size ) -1 ), std::runtime_error );
}

BOOST_AUTO_TEST_CASE( empty_chunked_body )
{
    body_receiver decoder( 100 );
    const char empty_chunk[] = "0\r\n\r\n";

    BOOST_CHECK_EQUAL( decoder.feed_chunked_buffer( &empty_chunk[ 0 ], sizeof( empty_chunk ) -1 ), sizeof( empty_chunk ) -1 );
    BOOST_CHECK( decoder.body().empty() );
}

BOOST_AUTO_TEST_CASE( will_read_behind_the_end_of_an_empty_body )
{
    BOOST_CHECK_EQUAL( test_body_decode_variations(
         "0\r\n\r\n123123",
         "" ), 6 );
}


static const char encoded_test_body[] =
    "29\r\n"
    "<html><body><p>The file you requested is \r\n"
    "5;foobar\r\n"
    "3,400\r\n"
    "22\r\n"
    "bytes long and was last modified: \r\n"
    "1d\r\n"
    "Sat, 20 Mar 2004 21:12:00 GMT\r\n"
    "13\r\n"
    ".</p></body></html>\r\n"
    "0\r\n"
    "Expires: Sat, 27 Mar 2004 21:12:00 GMT\r\n"
    "\r\n";

BOOST_AUTO_TEST_CASE( chunk_encoded_body )
{
    BOOST_CHECK_EQUAL( test_body_decode_variations(
        encoded_test_body,
        "<html><body><p>The file you requested is 3,400bytes long and was last modified: Sat, 20 Mar 2004 21:12:00 GMT.</p></body></html>" ), 0 );
}

BOOST_AUTO_TEST_CASE( chunk_encoded_step_by_step )
{
    static const char test_body[] =
        "29\r\n"
        "<html><body><p>The file you requested is \r\n"
        "5;foobar\r\n"
        "3,400\r\n"
        "22\r\n"
        "bytes long and was last modified: \r\n"
        "0\r\n"
        "\r\n";

    const char* current = &test_body[ 0 ];
    const char* end     = current + sizeof( test_body ) -1;

    body_receiver decoder( 32 );
    BOOST_CHECK( !decoder.chunked_done() );

    std::size_t fed = decoder.feed_chunked_buffer( current, 50 );
    BOOST_CHECK_EQUAL( fed, 32 + 4);
    current += fed;
    BOOST_CHECK( !decoder.chunked_done() );
    BOOST_CHECK_EQUAL( decoder.body_str(), "<html><body><p>The file you requ");

    fed = decoder.feed_chunked_buffer( current, 5 );
    BOOST_CHECK_EQUAL( fed, 5 );
    current += fed;
    BOOST_CHECK( !decoder.chunked_done() );
    BOOST_CHECK_EQUAL( decoder.body_str(), "<html><body><p>The file you requested");

    fed = decoder.feed_chunked_buffer( current, end - current );
    BOOST_CHECK_EQUAL( fed, 4 );
    current += fed;
    BOOST_CHECK( !decoder.chunked_done() );
    BOOST_CHECK_EQUAL( decoder.body_str(), "<html><body><p>The file you requested is ");

    fed = decoder.feed_chunked_buffer( current, end - current );
    BOOST_CHECK_EQUAL( fed, 17 );
    current += fed;
    BOOST_CHECK( !decoder.chunked_done() );
    BOOST_CHECK_EQUAL( decoder.body_str(), "<html><body><p>The file you requested is 3,400");

    fed = decoder.feed_chunked_buffer( current, end - current );
    BOOST_CHECK_EQUAL( fed, 38 );
    current += fed;
    BOOST_CHECK( !decoder.chunked_done() );
    BOOST_CHECK_EQUAL( decoder.body_str(), "<html><body><p>The file you requested is 3,400bytes long and was last modified");

    fed = decoder.feed_chunked_buffer( current, end - current );
    BOOST_CHECK_EQUAL( fed, 2 );
    current += fed;
    BOOST_CHECK( !decoder.chunked_done() );
    BOOST_CHECK_EQUAL( decoder.body_str(), "<html><body><p>The file you requested is 3,400bytes long and was last modified: ");

    fed = decoder.feed_chunked_buffer( current, end - current );
    BOOST_CHECK_EQUAL( fed, 7 );
    current += fed;
    BOOST_CHECK( decoder.chunked_done() );
    BOOST_CHECK_EQUAL( decoder.body_str(), "<html><body><p>The file you requested is 3,400bytes long and was last modified: ");
}

BOOST_AUTO_TEST_CASE( will_not_consume_behind_the_end )
{
    BOOST_CHECK_EQUAL( test_body_decode_variations(
        "29\r\n"
        "<html><body><p>The file you requested is \r\n"
        "5;foobar\r\n"
        "3,400\r\n"
        "22\r\n"
        "bytes long and was last modified: \r\n"
        "1d\r\n"
        "Sat, 20 Mar 2004 21:12:00 GMT\r\n"
        "13\r\n"
        ".</p></body></html>\r\n"
        "0\r\n"
        "Expires: Sat, 27 Mar 2004 21:12:00 GMT\r\n"
        "\r\n"
        "123",
        "<html><body><p>The file you requested is 3,400bytes long and was last modified: Sat, 20 Mar 2004 21:12:00 GMT.</p></body></html>" ), 3 );
}
