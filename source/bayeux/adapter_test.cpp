// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include <boost/test/unit_test.hpp>

#include "bayeux/adapter.h"

namespace tools
{
    template < class DataType >
    class adapter : public bayeux::adapter< DataType >
    {
    public:
        adapter()
        {
        }

        explicit adapter( const DataType& initial_data )
            : session_data_( initial_data )
        {
        }

    private:
        virtual std::pair< bool, std::string > handshake( const json::value& ext, DataType& client )
        {
            client = session_data_;
        }

        virtual std::pair< bool, std::string > publish( const json::string& channel, const json::value& data,
            DataType& client, pubsub::root& root )
        {
            session_data_ = client;
        }

        DataType    session_data_;

    };
}

BOOST_AUTO_TEST_CASE( handshake_hook_is_called )
{
    /*
    test_context context;

    const json::array response = bayeux_messages( bayeux_session(
        server::test::read_plan()
            << bayeux_msg(
                "{ 'channel' : '/meta/handshake',"
                "  'version' : '1.0.0',"
                "  'supportedConnectionTypes' : ['long-polling', 'callback-polling'],"
                "  'id'      : 'connect_id' }" )
            << bayeux_msg(
                "{ 'channel' : '/meta/subscribe',"
                "  'clientId' : '192.168.210.1:9999/0',"
                "  'subscription' : '/foo/bar' }" )
            << bayeux_msg(
                "{ 'channel' : '/meta/connect',"
                "  'clientId' : '192.168.210.1:9999/0',"
                "  'connectionType' : 'long-polling' }" )
            << server::test::disconnect_read(),
        context ) );

    BOOST_REQUIRE_EQUAL( 3u, response.length() );

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
        "       'channel'       : '/meta/subscribe',"
        "       'clientId'      : '192.168.210.1:9999/0',"
        "       'successful'    : true,"
        "       'subscription'  : '/foo/bar'"
        "   },"
        "   {"
        "       'channel'       : '/meta/connect',"
        "       'clientId'      : '192.168.210.1:9999/0',"
        "       'successful'    : true"
        "   }"
        "]" ) );*/
}

BOOST_AUTO_TEST_CASE( handshake_hook_result_is_applied )
{
}

BOOST_AUTO_TEST_CASE( handshake_hook_ext_is_transported )
{
}

BOOST_AUTO_TEST_CASE( publish_hook_is_called )
{
}

BOOST_AUTO_TEST_CASE( publish_hook_result_is_applied )
{
}

BOOST_AUTO_TEST_CASE( session_data_is_transported )
{
}
