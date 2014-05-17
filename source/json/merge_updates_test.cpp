#include <boost/test/unit_test.hpp>
#include "json/delta.h"
#include "json/json.h"

BOOST_AUTO_TEST_SUITE( merge_updates )

namespace {
    json::array parse( const char* t )
    {
        return json::parse_single_quoted( t ).upcast< json::array >();
    }
}

BOOST_AUTO_TEST_CASE( merge_delete_behind_delete )
{
    BOOST_CHECK_EQUAL(
        json::merge_updates(
            parse( "[2,1]" ),
            parse( "[2,3]" ) ),
        parse( "[2,1,2,2]" ) );
}

BOOST_AUTO_TEST_SUITE_END()
