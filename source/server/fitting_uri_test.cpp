#include <boost/test/unit_test.hpp>
#include "server/fitting_uri.h"

namespace {
    boost::test_tools::predicate_result test_fitting( const std::string& filter, const std::string& uri, bool expected )
    {
        server::fitting_uri fitter( tools::substring( uri.data(), uri.data() + uri.size() ) );

        if ( fitter( filter ) != expected )
        {
            boost::test_tools::predicate_result result( false );
            result.message() << "Expect uri: " << uri << " to fit " << (expected ? "" : "not ") << "the filter: " << filter;

            return result;
        }

        return true;
    }
    void fits( const std::string& filter, const std::string& uri )
    {
        BOOST_CHECK( test_fitting( filter, uri, true ) );
    }

    void fits_not( const std::string& filter, const std::string& uri )
    {
        BOOST_CHECK( test_fitting( filter, uri, false ) );
    }
}

BOOST_AUTO_TEST_SUITE( fitting_uri )

BOOST_AUTO_TEST_CASE( empty_filter_fits_to_all_uris )
{
    fits( "/", "/" );
}

BOOST_AUTO_TEST_CASE( exactly_fitting_fits )
{
    fits( "/pubsub", "/pubsub" );
    fits( "/pubsub/foo/bar", "/pubsub/foo/bar" );
}

BOOST_AUTO_TEST_CASE( fits_with_an_extra_slash )
{
    fits( "/pubsub", "/pubsub/" );
    fits( "/pubsub/", "/pubsub" );
    fits( "/pubsub/", "/pubsub/" );
    fits( "/pubsub/foo/bar", "/pubsub/foo/bar/" );
    fits( "/pubsub/foo/bar/", "/pubsub/foo/bar" );
    fits( "/pubsub/foo/bar/", "/pubsub/foo/bar/" );
}

BOOST_AUTO_TEST_CASE( fitting_at_the_start )
{
    fits( "/", "/pubsub" );
    fits( "/pubsub", "/pubsub/foo/bar" );
    fits( "/pubsub/foo", "/pubsub/foo/bar" );
    fits( "/pubsub/foo", "/pubsub/foo/bar.html" );
}

BOOST_AUTO_TEST_CASE( shorter_uris_doesnt_fit )
{
    fits_not( "/pubsub", "/" );
    fits_not( "/a/b/c", "/a/b" );
}

BOOST_AUTO_TEST_CASE( totally_unrelated_uris_doesnt_fit )
{
    fits_not( "Hallo", "Hello" );
    fits_not( "/foo", "/bar" );
    fits_not( "abc", "cba" );
}

BOOST_AUTO_TEST_CASE( a_path_segment_has_to_fit_exactly )
{
    fits_not( "/ab", "/abc" );
    fits_not( "/abc", "/ab" );
    fits_not( "/ab/cd", "/ab/cde" );
    fits_not( "/ab/cde", "/ab/cd" );
    fits_not( "/ab/cd/ef", "/abcdef" );
    fits_not( "/ab/cd/ef", "/ab/cd/e" );
}

BOOST_AUTO_TEST_SUITE_END()

