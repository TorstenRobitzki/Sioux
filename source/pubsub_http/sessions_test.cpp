#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/bind.hpp>
#include "pubsub/root.h"
#include "pubsub/configuration.h"
#include "pubsub/test_helper.h"
#include "pubsub_http/sessions.h"
#include "pubsub_http/response_decl.h"
#include "server/test_session_generator.h"
#include "asio_mocks/test_timer.h"
#include "tools/io_service.h"

namespace {

    class waiting_connection_impl : public pubsub::http::waiting_connection
    {
    public:
        waiting_connection_impl()
            : second_connection_called_( false )
            , update_called_( false )
        {
        }

        bool second_connection_called()
        {
            boost::mutex::scoped_lock lock( mutex_ );
            const bool result = second_connection_called_;
            second_connection_called_ = false;

            return result;
        }

        bool update_called()
        {
            boost::mutex::scoped_lock lock( mutex_ );
            const bool result = update_called_;
            update_called_ = false;

            return result;
        }

        json::array updates()
        {
            boost::mutex::scoped_lock lock( mutex_ );
            json::array result;

            updates_.swap( result );
            return result;
        }

        json::array response()
        {
            boost::mutex::scoped_lock lock( mutex_ );
            json::array result;

            response_.swap( result );
            return result;
        }
    private:
        boost::mutex    mutex_;
        bool            second_connection_called_;
        bool            update_called_;

        json::array     updates_;
        json::array     response_;

        virtual void second_connection()
        {
            boost::mutex::scoped_lock lock( mutex_ );
            second_connection_called_ = true;
        }

        virtual void update( const json::array& response, const json::array& update )
        {
            boost::mutex::scoped_lock lock( mutex_ );
            update_called_ = true;
            updates_ = update;
            response_ = response;
        }
    };

    // data, we don't want to inherit from
    struct root_data
    {
        root_data( boost::asio::io_service& queue, pubsub::adapter& adapter, const pubsub::configuration& default_configuration )
            : root_( queue, adapter, default_configuration )
        {
        }

        server::test::session_generator generator_;
        pubsub::root                    root_;
    };

    struct context
        : boost::asio::io_service
        , pubsub::test::adapter
        , root_data
        , pubsub::http::sessions< asio_mocks::timer >
    {
        context()
            : boost::asio::io_service()
            , pubsub::test::adapter( static_cast< boost::asio::io_service& >( *this ) )
            , root_data( *this, *this, pubsub::configuration() )
            , pubsub::http::sessions< asio_mocks::timer >( generator_, *this, root_data::root_ )
            , default_id( "net/0" )
            , default_network( "net" )
            , test_start( asio_mocks::current_time() )
        {
        }

        void wait_for_session_timeout()
        {
            asio_mocks::advance_time( boost::posix_time::seconds( 21 ) );
            tools::run( *this );
        }

        boost::posix_time::time_duration elapsed_time() const
        {
            return asio_mocks::current_time() - test_start;
        }

        const json::string              default_id;
        const std::string               default_network;
        const boost::posix_time::ptime  test_start;
    };

    struct context_with_old_session : context
    {
        context_with_old_session()
        {
            session = find_or_create_session( default_id, default_network );
            const json::string session_id = pubsub::http::id( session.first );
            idle_session( session.first );

            session = find_or_create_session( session_id, default_network );
        }

        ~context_with_old_session()
        {
            if ( session.first )
                idle_session( session.first );
        }

        std::pair< pubsub::http::session_impl*, bool > session;
    };


    struct context_with_waiting_session : context_with_old_session
    {
        context_with_waiting_session()
            : update( new waiting_connection_impl )
        {
            wait_for_session_event( session.first, update );
        }

        boost::shared_ptr< waiting_connection_impl >   update;
    };

    static const pubsub::node_name node_a( json::parse_single_quoted( "{ 'name': 'a' }" ).upcast< json::object >() );
    static const pubsub::node_name node_b( json::parse_single_quoted( "{ 'name': 'b' }" ).upcast< json::object >() );

    struct context_with_subscribed_session : context_with_waiting_session
    {
        context_with_subscribed_session()
        {
            answer_validation_request( node_a, true );
            answer_authorization_request( node_a, true );
            answer_initialization_request( node_a, json::string( "Hello Subscriber") );

            subscribe( session.first, node_a );

            tools::run( *this );
        }
    };

    // searches for an object in arr, that contains at least all key/value pairs, that obj contains
    bool find_object( const json::array& arr, const json::object& obj )
    {
        struct compare_obj : public json::default_visitor
        {
            explicit compare_obj( const json::object& obj )
                : obj_( obj )
                , keys_( obj_.keys() )
                , found( false )
            {}

            bool close_to( const json::object& other ) const
            {
                bool failed = false;
                for ( std::vector< json::string >::const_iterator key = keys_.begin(); key != keys_.end() && !failed; ++key )
                {
                    const json::value* const val = other.find( *key );
                    failed = val == 0 || obj_.at( *key ) != *val;
                }

                return !failed;
            }

            void visit( const json::object& other )
            {
                found = found || close_to( other );
            }

            const json::object&         obj_;
            std::vector< json::string > keys_;
            bool                        found;
        } compare( obj );

        arr.for_each( compare );

        return compare.found;
    }

    bool find_object( const json::array& arr, const char* single_quoted_obj )
    {
        return find_object( arr, json::parse_single_quoted( single_quoted_obj ).upcast< json::object >() );
    }
}

BOOST_AUTO_TEST_CASE( find_object_test )
{
    const json::array test_data = json::parse_single_quoted("["
            "1,"
            "'asdasd',"
            "{ 'a': 1, 'b': 'b', 'c': false },"
            "42,"
            "{ 'a': 2 },"
            "{ 'b': 2, 'c': 3 }"
        "]").upcast< json::array >();

    BOOST_CHECK( find_object( test_data, "{ 'a': 1 }" ) );
    BOOST_CHECK( find_object( test_data, "{ 'a': 2 }" ) );
    BOOST_CHECK( find_object( test_data, "{ 'c': 3 }" ) );
    BOOST_CHECK( find_object( test_data, "{ 'b': 'b' }" ) );
    BOOST_CHECK( find_object( test_data, "{ 'a': 1, 'b': 'b', 'c': false }" ) );

    BOOST_CHECK( !find_object( test_data, "{ 'a': 17 }" ) );
    BOOST_CHECK( !find_object( test_data, "{ 'a': 1, 'b': 2 }" ) );
}

BOOST_FIXTURE_TEST_CASE( accessing_a_session_perodically_within_the_timeout_period, context )
{
    for ( int times = 0; times != 10; ++times )
    {
        const std::pair< pubsub::http::session_impl*, bool > session = find_or_create_session(
            default_id, default_network );

        BOOST_CHECK_EQUAL( pubsub::http::id( session.first ), default_id );
        idle_session( session.first );

        asio_mocks::advance_time( boost::posix_time::seconds( 19 ) );
        tools::run( *this );

    }
}

BOOST_FIXTURE_TEST_CASE( accessing_a_session_perodically_after_the_timeout_period, context )
{
    json::string last_id( "XXX" );

    for ( int times = 0; times != 10; ++times )
    {
        const std::pair< pubsub::http::session_impl*, bool > session = find_or_create_session(
            default_id, default_network );

        BOOST_CHECK_NE( pubsub::http::id( session.first ), last_id );
        BOOST_CHECK( session.second );
        last_id = pubsub::http::id( session.first );
        idle_session( session.first );

        wait_for_session_timeout();
    }
}

BOOST_FIXTURE_TEST_CASE( asking_for_a_new_session_yields_a_new_session, context )
{
    const std::pair< pubsub::http::session_impl*, bool > result =
        find_or_create_session( json::string( "abc" ), default_network );

    BOOST_CHECK( result.first );
    BOOST_CHECK( result.second );
}

BOOST_FIXTURE_TEST_CASE( asking_for_a_old_session_yields_the_old_session, context )
{
    const std::pair< pubsub::http::session_impl*, bool > session =
        find_or_create_session( json::string( "abc" ), default_network );

    const json::string session_id = pubsub::http::id( session.first );
    idle_session( session.first );

    const std::pair< pubsub::http::session_impl*, bool > session_again =
        find_or_create_session( session_id, default_network );

    BOOST_CHECK( !session_again.second );
    BOOST_CHECK_EQUAL( session_id, pubsub::http::id( session_again.first ) );
}

BOOST_FIXTURE_TEST_CASE( using_an_empty_session_id_results_always_in_a_new_session, context )
{
    const std::pair< pubsub::http::session_impl*, bool > session1 =
        find_or_create_session( json::string(), default_network );

    const std::pair< pubsub::http::session_impl*, bool > session2 =
        find_or_create_session( json::string(), default_network );

    BOOST_CHECK_NE( pubsub::http::id( session1.first ), pubsub::http::id( session2.first) );
}

BOOST_FIXTURE_TEST_CASE( a_session_in_use_will_not_timeout, context )
{
    {
        const std::pair< pubsub::http::session_impl*, bool > session =
            find_or_create_session( json::string( "abc" ), default_network );

        asio_mocks::advance_time( boost::posix_time::seconds( 120 ) );

        tools::run( *this );
        idle_session( session.first );
    }

    const std::pair< pubsub::http::session_impl*, bool > result =
        find_or_create_session( default_id, default_network );

    BOOST_CHECK( !result.second );
    BOOST_CHECK_EQUAL( default_id, pubsub::http::id( result.first ) );
}

BOOST_FIXTURE_TEST_CASE( wait_and_wakeup, context_with_waiting_session )
{
    BOOST_CHECK( wake_up( session.first, update) );
}

BOOST_FIXTURE_TEST_CASE( wait_and_wakeup_twice, context_with_waiting_session )
{
    wake_up( session.first, update);
    BOOST_CHECK( !wake_up( session.first, update) );
}

BOOST_FIXTURE_TEST_CASE( second_session_will_be_detected, context_with_waiting_session )
{
    std::pair< pubsub::http::session_impl*, bool > second_session = find_or_create_session( default_id, default_network );
    BOOST_CHECK_EQUAL( pubsub::http::id( session.first ), pubsub::http::id( second_session.first ) );

    boost::shared_ptr< waiting_connection_impl > second_update( new waiting_connection_impl );
    wait_for_session_event( second_session.first, second_update );

    BOOST_CHECK( update->second_connection_called() );
    BOOST_CHECK( !second_update->second_connection_called() );
}

BOOST_FIXTURE_TEST_CASE( a_subscribed_connection_will_receive_updates, context_with_waiting_session )
{
    answer_validation_request( node_a, true );
    answer_authorization_request( node_a, true );
    answer_initialization_request( node_a, json::string( "Hello Subscriber") );

    subscribe( session.first, node_a );

    tools::run( *this );
    BOOST_CHECK( update->update_called() );

    const json::array arr = update->updates();
    BOOST_REQUIRE_EQUAL( arr.length(), 1u );
    BOOST_CHECK( find_object( arr, "{ 'key': { 'name': 'a' }, 'data': 'Hello Subscriber' }" ) );

    BOOST_CHECK( update->response().empty() );
}

BOOST_FIXTURE_TEST_CASE( a_subscribed_connection_will_stop_receiving_updates, context_with_subscribed_session )
{
    update->update_called();
    tools::run( *this );
    BOOST_CHECK( !update->update_called() );

    wait_for_session_event( session.first, update );
    unsubscribe( session.first, node_a );
    tools::run( *this );

    root_data::root_.update_node( node_a, json::number( 42 ) );
    tools::run( *this );

    BOOST_CHECK( !update->update_called() );
}

BOOST_FIXTURE_TEST_CASE( updates_comming_while_not_waiting, context )
{
}

BOOST_FIXTURE_TEST_CASE( invalid_node_subscription_will_be_reived, context )
{
}

BOOST_FIXTURE_TEST_CASE( invalid_node_subscription_while_not_waiting, context )
{
}

BOOST_FIXTURE_TEST_CASE( authorization_failure_will_be_reived, context )
{
}

BOOST_FIXTURE_TEST_CASE( authorization_failure_while_not_waiting, context )
{
}
