
#include <boost/test/unit_test.hpp>
#include "tools/hexdump.h"

#include <sstream>

BOOST_AUTO_TEST_CASE(dumping_a_single_char_doesnt_result_in_an_infinit_loop)
{
    std::ostringstream out;
    tools::print_hex( out, 'a' );
}

BOOST_AUTO_TEST_CASE( print_hex_returns_hex_codes )
{
    std::ostringstream out;
    tools::print_hex( out, '\0' );
    tools::print_hex( out, static_cast< char >( 0xffu ) );
    BOOST_CHECK_EQUAL( out.str(), "00ff" );
}
