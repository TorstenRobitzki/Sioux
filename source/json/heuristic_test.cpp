#include <boost/test/unit_test.hpp>
#include "json/internal/heuristic.h"
#include "json/json.h"
#include <cmath>

BOOST_AUTO_TEST_SUITE( heuristic )

namespace {

    struct context 
    {
        const json::array a;
        const json::array b;
        json::details::heuristic h;

        context() : h( a, b )
        {
        }

        explicit context( const json::array both ) 
            : a( both )
            , b( both )
            , h( a, b )
        {
        }

        context( const json::array& from, const json::array& to )
            : a( from )
            , b( to )
            , h( a, b )
        {
        }
    };

    struct heuristic_with_empty_arrays : context 
    {
    };
}

BOOST_FIXTURE_TEST_CASE( zero_difference, heuristic_with_empty_arrays )
{
    BOOST_CHECK_EQUAL( h( 0, 0 ), 0 );
}

namespace {

    static const json::array ten_1s = 
        json::parse_single_quoted( "[1,1,1,1,1,1,1,1,1,1]" ).upcast< json::array >();

    static const json::array one_to_10 =
        json::parse_single_quoted( "[1,2,3,4,5,6,7,8,9,10]" ).upcast< json::array >();

    struct heuristic_with_10_1s : context
    {
        heuristic_with_10_1s() : context( ten_1s )
        {
        }
    };

}

BOOST_FIXTURE_TEST_CASE( zero_difference_at_the_end, heuristic_with_10_1s )
{
    BOOST_CHECK_EQUAL( h( 10u, 10u ), 0 );
}

BOOST_FIXTURE_TEST_CASE( estimate_zero_at_the_beginning, heuristic_with_10_1s )
{
    BOOST_CHECK_EQUAL( h( 0, 0 ), 0 );
}

BOOST_FIXTURE_TEST_CASE( estimate_zero_in_the_middle, heuristic_with_10_1s )
{
    BOOST_CHECK_EQUAL( h( 5u, 5u ), 0 );
}

BOOST_FIXTURE_TEST_CASE( estimate_a_distance_of_one, heuristic_with_10_1s )
{
//    BOOST_CHECK_EQUAL( h( 0u, 1u ), std::string( ",1" ).size() );
}

namespace {
    struct heuristic_with_one_empty_one_1_to_10 : context
    {
        heuristic_with_one_empty_one_1_to_10() : context( json::array(), one_to_10 ) {}
    };
}

/*
 * if the a array is empty, the estimation is the length of b beginning with b_index
 */
BOOST_FIXTURE_TEST_CASE( index_runs_with_length, heuristic_with_one_empty_one_1_to_10 )
{
    BOOST_CHECK_EQUAL( h( 0, 10 ), std::string( "" ).size() );
    BOOST_CHECK_EQUAL( h( 0, 9 ), std::string( ",10" ).size() );
    BOOST_CHECK_EQUAL( h( 0, 7 ), std::string( ",8,9,10" ).size() );
    BOOST_CHECK_EQUAL( h( 0, 5 ), std::string( ",6,7,8,9,10" ).size() );
    BOOST_CHECK_EQUAL( h( 0, 1 ), std::string( ",2,3,4,5,6,7,8,9,10" ).size() );
    BOOST_CHECK_EQUAL( h( 0, 0 ), std::string( "1,2,3,4,5,6,7,8,9,10" ).size() );
}

namespace {
    struct heuristic_with_two_one_1_to_10 : context
    {
        heuristic_with_two_one_1_to_10() : context( one_to_10, one_to_10 ) {}
    };
}

BOOST_FIXTURE_TEST_CASE( shorter_a_than_b, heuristic_with_two_one_1_to_10 )
{
    BOOST_CHECK_EQUAL( h( 7, 4 ), std::string( ",5,6,7,8,9,19" ).size() - std::string( ",8,9,10" ).size() );
    BOOST_CHECK_EQUAL( h( 3, 0 ), std::string( "1,2,3,4,5,6,7,8,9,19" ).size() - std::string( ",4,5,6,7,8,9,10" ).size() );
    BOOST_CHECK_EQUAL( h( 10, 9 ), std::string( ",10" ).size() - std::string( "" ).size() );
}

BOOST_FIXTURE_TEST_CASE( equal_index_results_in_zero_costs, heuristic_with_two_one_1_to_10 )
{
    BOOST_CHECK_EQUAL( h( 0, 0 ), 0 );
    BOOST_CHECK_EQUAL( h( 10, 10 ), 0 );
    BOOST_CHECK_EQUAL( h( 5, 5 ), 0 );
}

BOOST_FIXTURE_TEST_CASE( larger_a_than_b, heuristic_with_two_one_1_to_10 )
{
    BOOST_CHECK_EQUAL( h( 4, 7 ), 4u );
    BOOST_CHECK_EQUAL( h( 0, 3 ), 4u );
    BOOST_CHECK_EQUAL( h( 9, 10 ), 4u );
}

BOOST_AUTO_TEST_SUITE_END()



