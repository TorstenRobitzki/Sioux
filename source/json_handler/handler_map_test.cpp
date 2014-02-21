
#include "json_handler/handler_map.h"

#include <boost/test/unit_test.hpp>
#include <boost/bind.hpp>

namespace
{
    static const std::vector< std::string > all_urls;
    static const std::vector< http::http_method_code > all_methods;
    static const http::request_header empty_request;

    struct context : json::detail::handler_map
    {
        std::pair< json::value, http::http_error_code > fa( const http::request_header&, const json::value& )
        {
            static const std::pair< json::value, http::http_error_code > result( json::null(), http::http_ok );
            handler_a_called = true;

            return result;
        }

        context()
            : handler_a_called( false )
            , handler_a( boost::bind( &context::fa, this, _1, _2 ) )
        {
        }

        bool                                handler_a_called;
        const json::connector::handler_t    handler_a;
    };
}

BOOST_FIXTURE_TEST_CASE( will_return_null_if_empty, json::detail::handler_map )
{
    BOOST_CHECK( find_handler( "", http::http_get ) == 0 );
}

BOOST_FIXTURE_TEST_CASE( a_default_handler_will_be_found, context )
{
    add_handler( all_urls, all_methods, handler_a );
    const json::connector::handler_t* handler = find_handler( "/foobar", http::http_get );

   BOOST_REQUIRE( handler != 0 );


   (*handler)( empty_request, json::null() );
   BOOST_CHECK( handler_a_called );
}
