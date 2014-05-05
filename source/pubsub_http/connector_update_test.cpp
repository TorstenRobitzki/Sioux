#include "pubsub_http/connector.h"
#include "test_context.h"

namespace {

    struct subscribed : context 
    {

        json::object subscribe_and_wait_for_data()
        {
            json::object response = json_body( 
                http_post( subscribe_to_node1( json::parse_single_quoted("[1,2,3,4,5,6,7,8,9,10]") ) << asio_mocks::disconnect_read(), 1 ) );

            static const json::string data_key("update");

            const json::value* update = 0;
            for ( int retries = 0; retries != 5 && !update; ++retries, update = response.find( data_key ) )
            {
                response = json_body( http_post( poll() << asio_mocks::disconnect_read(), 1 ) );
            }

            if ( !update )
                throw std::runtime_error("no update found in subscribe_and_wait_for_data");

            return update->upcast< json::array >().at( 0 ).upcast< json::object >();             
        }

        subscribed()
            : first_version( 1 )
            , next_version( 1 )
        {

            const json::object update = subscribe_and_wait_for_data();

            first_version = update
                .at( "version" )
                .upcast< json::number >();
            next_version = json::number( first_version.to_int() +1 );
        }

        json::number first_version;
        json::number next_version;
    };
}

BOOST_FIXTURE_TEST_CASE( a_subscribed_client_gets_updates, subscribed )
{
    data_.update_node( node1, json::parse_single_quoted("[1,2,4,4,5,6,7,8,9,10]") );

    const json::array response = json_multiple_post(
           asio_mocks::json_msg(
            "{"
            "   'id': '192.168.210.1:9999/0'"
            "}" )
        << asio_mocks::disconnect_read() );

    json::value expected = json::parse_single_quoted(
            "{"
            "   'id': '192.168.210.1:9999/0',"
            "   'update': [{"
            "       'key': { 'a': '1', 'b': '1' },"
            "       'update': [1,2,4],"
            "       'from': 999,"
            "       'version': 999,"
            "   }]"
            "}");
    expected = replace_version( expected, next_version );
    expected = replace_from( expected, first_version );

    BOOST_REQUIRE_EQUAL( response.length(), 1u );
    BOOST_CHECK_EQUAL( response.at( 0 ), expected );
}


