#include <boost/test/unit_test.hpp>

#include "bayeux/adapter.h"
#include "bayeux/test_tools.h"
#include "asio_mocks/json_msg.h"

namespace test
{
    // Adapter for testing
    template < class DataType >
    class adapter : public bayeux::adapter< DataType >
    {
    public:
        adapter()
            : handshake_( false )
            , handshake_ret_value_( true, json::string() )
            , handshake_session_data_out_( DataType() )
            , handshake_ext_( json::null() )
            , published_( false )
            , published_ret_value_( true, json::string() )
            , published_session_data_out_( DataType() )
            , published_data_( json::null() )
        {
        }

        void handshake_result( const std::pair< bool, json::string >& ret_value, const DataType& session_data )
        {
            handshake_ret_value_        = ret_value;
            handshake_session_data_out_ = session_data;
        }

        bool handshake_called( const json::value& ext, const DataType& client )
        {
            const bool result = handshake_ && handshake_ext_ == ext && handshake_session_data_in_ == client;
            handshake_ = false;

            return result;
        }

        void publish_result( const std::pair< bool, json::string >& ret_value, const DataType& session_data )
        {
            published_ret_value_        = ret_value;
            published_session_data_out_ = session_data;
        }

        bool published_called( const json::string& channel, const json::value& data, const json::object& message,
            const DataType& session_data )
        {
            const bool result = published_ && published_channel_ == channel && published_data_ == data
                && published_message_ == message && published_session_data_in_ == session_data;
            published_ = false;

            return result;
        }

        bool published_called( const json::string& channel, const json::value& data,
            const DataType& session_data )
        {
            const bool result = published_ && published_channel_ == channel && published_data_ == data
                && published_session_data_in_ == session_data;
            published_ = false;

            return result;
        }
    private:
        virtual std::pair< bool, json::string > handshake( const json::value& ext, DataType& client )
        {
            BOOST_REQUIRE( !handshake_ );
            handshake_                  = true;
            handshake_ext_              = ext;
            handshake_session_data_in_  = client;

            client = handshake_session_data_out_;
            return handshake_ret_value_;
        }

        virtual std::pair< bool, json::string > publish( const json::string& channel, const json::value& data,
            const json::object& message, DataType& client, pubsub::root& root )
        {
            BOOST_REQUIRE( !published_ );
            published_                  = true;
            published_channel_          = channel;
            published_data_             = data;
            published_session_data_in_  = client;
            published_message_          = message;

            client = published_session_data_out_;
            return published_ret_value_;
        }

        bool                            handshake_;
        std::pair< bool, json::string > handshake_ret_value_;
        DataType                        handshake_session_data_out_;
        json::value                     handshake_ext_;
        DataType                        handshake_session_data_in_;
        json::object                    published_message_;

        bool                            published_;
        std::pair< bool, json::string > published_ret_value_;
        DataType                        published_session_data_out_;
        DataType                        published_session_data_in_;
        json::string                    published_channel_;
        json::value                     published_data_;
    };
}

BOOST_AUTO_TEST_CASE( handshake_hook_is_called )
{
    test::adapter< json::string > adapter;

    bayeux::test::context context( adapter );

    const json::array response = bayeux::test::bayeux_messages( bayeux::test::bayeux_session(
        asio_mocks::read_plan()
            << asio_mocks::json_msg(
                "{ 'channel' : '/meta/handshake',"
                "  'version' : '1.0.0',"
                "  'supportedConnectionTypes' : ['long-polling', 'callback-polling'],"
                "  'id'      : 'connect_id' }" )
            << asio_mocks::disconnect_read(),
        context ) );

    BOOST_CHECK_EQUAL( response, json::parse_single_quoted(
        "["
        "   {"
        "       'channel'       : '/meta/handshake',"
        "       'version'       : '1.0',"
        "       'clientId'      : '192.168.210.1:9999/0',"
        "       'successful'    : true,"
        "       'supportedConnectionTypes': ['long-polling'],"
        "       'id'            : 'connect_id'"
        "   }"
        "]" ) );

    BOOST_CHECK( adapter.handshake_called( json::null(), json::string() ) );
}

BOOST_AUTO_TEST_CASE( handshake_hook_result_is_applied )
{
    test::adapter< json::string > adapter;
    adapter.handshake_result( std::make_pair( false, json::string( "you can not pass!" ) ), json::string() );

    bayeux::test::context context( adapter );

    const json::array response = bayeux::test::bayeux_messages( bayeux::test::bayeux_session(
        asio_mocks::read_plan()
            << asio_mocks::json_msg(
                "{ 'channel' : '/meta/handshake',"
                "  'version' : '1.0.0',"
                "  'supportedConnectionTypes' : ['long-polling', 'callback-polling'],"
                "  'id'      : 'connect_id' }" )
            << asio_mocks::disconnect_read(),
        context ) );

    BOOST_CHECK_EQUAL( response, json::parse_single_quoted(
        "["
        "   {"
        "       'channel'       : '/meta/handshake',"
        "       'successful'    : false,"
        "       'error'         : 'you can not pass!',"
        "       'id'            : 'connect_id'"
        "   }"
        "]" ) );

    BOOST_CHECK( adapter.handshake_called( json::null(), json::string() ) );
}

BOOST_AUTO_TEST_CASE( handshake_hook_ext_is_transported )
{
    test::adapter< json::string > adapter;

    bayeux::test::context context( adapter );

    const json::array response = bayeux::test::bayeux_messages( bayeux::test::bayeux_session(
        asio_mocks::read_plan()
            << asio_mocks::json_msg(
                "{ 'channel' : '/meta/handshake',"
                "  'version' : '1.0.0',"
                "  'supportedConnectionTypes' : ['long-polling', 'callback-polling'],"
                "  'id'      : 'connect_id',"
                "  'ext'     : 'foobar' }" )
            << asio_mocks::disconnect_read(),
        context ) );

    BOOST_CHECK_EQUAL( response, json::parse_single_quoted(
        "["
        "   {"
        "       'channel'       : '/meta/handshake',"
        "       'version'       : '1.0',"
        "       'clientId'      : '192.168.210.1:9999/0',"
        "       'successful'    : true,"
        "       'supportedConnectionTypes': ['long-polling'],"
        "       'id'            : 'connect_id'"
        "   }"
        "]" ) );

    BOOST_CHECK( adapter.handshake_called( json::string( "foobar" ), json::string() ) );
}

BOOST_AUTO_TEST_CASE( publish_hook_is_called )
{
    test::adapter< int > adapter;

    bayeux::test::context context( adapter );

    static const char publish_message[] = "{ "
        "   'channel'       : '/foo/bar',"
        "   'clientId'      : '192.168.210.1:9999/0',"
        "   'data'          : true,"
        "   'id'            : 42"
        "}";

    const json::array response = bayeux::test::bayeux_messages( bayeux::test::bayeux_session(
        asio_mocks::read_plan()
            << asio_mocks::json_msg(
                "{ "
                "   'channel' : '/meta/handshake',"
                "   'version' : '1.0.0',"
                "   'supportedConnectionTypes' : ['long-polling', 'callback-polling'],"
                "   'id'      : 'connect_id' }" )
            << asio_mocks::json_msg( publish_message )
            << asio_mocks::disconnect_read(),
        context ) );

    BOOST_CHECK_EQUAL( response, json::parse_single_quoted(
        "["
        "   {"
        "       'channel'       : '/meta/handshake',"
        "       'version'       : '1.0',"
        "       'clientId'      : '192.168.210.1:9999/0',"
        "       'successful'    : true,"
        "       'supportedConnectionTypes': ['long-polling'],"
        "       'id'            : 'connect_id'"
        "   },"
        "   {"
        "       'channel'       : '/foo/bar',"
        "       'successful'    : true,"
        "       'id'            : 42"
        "   }"
        "]" ) );

    BOOST_CHECK( adapter.published_called( json::string( "/foo/bar"), json::true_val(),
        json::parse_single_quoted( publish_message ).upcast< json::object >(), 0 ) );
}

BOOST_AUTO_TEST_CASE( publish_hook_result_is_applied )
{
    test::adapter< int > adapter;
    adapter.publish_result( std::make_pair( false, json::string( "come back later" ) ), 0 );

    bayeux::test::context context( adapter );

    const json::array response = bayeux::test::bayeux_messages( bayeux::test::bayeux_session(
        asio_mocks::read_plan()
            << asio_mocks::json_msg(
                "{ "
                "   'channel' : '/meta/handshake',"
                "   'version' : '1.0.0',"
                "   'supportedConnectionTypes' : ['long-polling', 'callback-polling'],"
                "   'id'      : 'connect_id' }" )
            << asio_mocks::json_msg(
                "{ "
                "   'channel'       : '/foo/bar',"
                "   'clientId'      : '192.168.210.1:9999/0',"
                "   'data'          : true,"
                "   'id'            : 42"
                "}" )
            << asio_mocks::disconnect_read(),
        context ) );

    BOOST_CHECK_EQUAL( response, json::parse_single_quoted(
        "["
        "   {"
        "       'channel'       : '/meta/handshake',"
        "       'version'       : '1.0',"
        "       'clientId'      : '192.168.210.1:9999/0',"
        "       'successful'    : true,"
        "       'supportedConnectionTypes': ['long-polling'],"
        "       'id'            : 'connect_id'"
        "   },"
        "   {"
        "       'channel'       : '/foo/bar',"
        "       'successful'    : false,"
        "       'error'         : 'come back later',"
        "       'id'            : 42"
        "   }"
        "]" ) );

    BOOST_CHECK( adapter.published_called( json::string( "/foo/bar"), json::true_val(), 0 ) );
}

BOOST_AUTO_TEST_CASE( session_data_is_transported )
{
    test::adapter< json::string > adapter;
    adapter.handshake_result( std::make_pair( true, json::string() ), json::string( "First Value" ) );

    bayeux::test::context context( adapter );

    bayeux::test::bayeux_session(
        asio_mocks::read_plan()
            << asio_mocks::json_msg(
                "{ "
                "   'channel' : '/meta/handshake',"
                "   'version' : '1.0.0',"
                "   'supportedConnectionTypes' : ['long-polling', 'callback-polling'],"
                "   'id'      : 'connect_id' }" )
            << asio_mocks::disconnect_read(),
            asio_mocks::write_plan(),
        context, boost::posix_time::time_duration() );

    BOOST_CHECK( adapter.handshake_called( json::null(), json::string( "" ) ) );
    adapter.publish_result( std::make_pair( true, json::string() ), json::string( "Second Value" ) );

    bayeux::test::bayeux_session(
        asio_mocks::read_plan()
            << asio_mocks::json_msg(
                "{ "
                "   'channel'       : '/foo/bar',"
                "   'clientId'      : '192.168.210.1:9999/0',"
                "   'data'          : true,"
                "   'id'            : 42"
                "}" )
            << asio_mocks::disconnect_read(),
        asio_mocks::write_plan(),
        context, boost::posix_time::time_duration() );

    BOOST_CHECK( adapter.published_called( json::string( "/foo/bar"), json::true_val(), json::string( "First Value" ) ) );

    bayeux::test::bayeux_session(
        asio_mocks::read_plan()
            << asio_mocks::json_msg(
                "{ "
                "   'channel'       : '/foo/bar',"
                "   'clientId'      : '192.168.210.1:9999/0',"
                "   'data'          : false,"
                "   'id'            : 42"
                "}" )
            << asio_mocks::disconnect_read(),
        context );

    BOOST_CHECK( adapter.published_called( json::string( "/foo/bar"), json::false_val(), json::string( "Second Value" ) ) );
}

