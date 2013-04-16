// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "asio_mocks/test_timer.h"
#include "tools/io_service.h"

namespace
{
    class timer_call_back : boost::noncopyable
    {
    public:
      timer_call_back()
          : expire_time_( )
          , error_()
          , called_( false )
      {
      }

      void operator()( const boost::system::error_code& error )
      {
          expire_time_ = asio_mocks::current_time();
          error_       = error;
          called_      = true;
      }

      void check_called_without_error_at( const boost::posix_time::ptime& time)
      {
          BOOST_CHECK( called_ );
          BOOST_CHECK( !error_ );
          BOOST_CHECK_EQUAL( time, expire_time_ );

          called_ = false;
      }

      void check_canceled()
      {
          BOOST_CHECK( called_ );
          BOOST_CHECK_EQUAL( error_, boost::asio::error::operation_aborted );

          called_ = false;
      }

      void check_not_called()
      {
          BOOST_CHECK( !called_ );
          called_ = false;
      }

    private:
      boost::posix_time::ptime  expire_time_;
      boost::system::error_code error_;
      bool                      called_;
    };
}

BOOST_AUTO_TEST_CASE( check_timer_test_equipment )
{
    timer_call_back xcb;
    xcb( boost::system::error_code() );
    xcb.check_called_without_error_at( asio_mocks::current_time() );

    BOOST_CHECK_EQUAL( boost::posix_time::time_from_string( "1970-01-01 00:00:00" ), asio_mocks::current_time() );

    asio_mocks::current_time( boost::posix_time::time_from_string( "2012-03-01 11:31:42" ) );
    BOOST_CHECK_EQUAL( boost::posix_time::time_from_string( "2012-03-01 11:31:42" ), asio_mocks::current_time() );
}

BOOST_AUTO_TEST_CASE( single_expiration_time )
{
    boost::asio::io_service queue;
    asio_mocks::timer     timer( queue );

    const boost::posix_time::ptime expected_time = asio_mocks::current_time() + boost::posix_time::seconds( 2 );
    BOOST_CHECK( timer.expires_at( expected_time ) == 0 );

    timer_call_back handler;
    timer.async_wait( boost::ref( handler ) );

    asio_mocks::current_time( expected_time - boost::posix_time::seconds( 1 ) );
    tools::run( queue );
    handler.check_not_called();

    asio_mocks::current_time( expected_time );
    tools::run( queue );
    handler.check_called_without_error_at( expected_time );
}

BOOST_AUTO_TEST_CASE( multiple_expiration_times )
{
    boost::asio::io_service queue;
    asio_mocks::timer     timer( queue );

    const boost::posix_time::ptime first_expected_time = asio_mocks::current_time() + boost::posix_time::seconds( 2 );
    BOOST_CHECK( timer.expires_at( first_expected_time - boost::posix_time::milliseconds( 1 ) ) == 0 );

    timer_call_back first_handler;
    timer.async_wait( boost::ref( first_handler ) );

    asio_mocks::current_time( first_expected_time );
    tools::run( queue );
    first_handler.check_called_without_error_at( first_expected_time );

    const boost::posix_time::ptime second_expected_time = first_expected_time + boost::posix_time::seconds( 2 );
    BOOST_CHECK( timer.expires_at( second_expected_time ) == 0 );

    timer_call_back second_handler;
    timer.async_wait( boost::ref( second_handler ) );

    asio_mocks::current_time( second_expected_time );
    tools::run( queue );
    second_handler.check_called_without_error_at( second_expected_time );
}

BOOST_AUTO_TEST_CASE( multiple_expiration_times_multiple_timers )
{
    boost::asio::io_service queue;
    asio_mocks::timer     first_timer( queue );
    asio_mocks::timer     second_timer( queue );

    const boost::posix_time::ptime first_expected_time  = asio_mocks::current_time() + boost::posix_time::seconds( 1 );
    const boost::posix_time::ptime second_expected_time = asio_mocks::current_time() + boost::posix_time::seconds( 2 );
    BOOST_CHECK( first_timer.expires_at( first_expected_time ) == 0 );
    BOOST_CHECK( second_timer.expires_at( second_expected_time ) == 0 );

    timer_call_back first_handler;
    first_timer.async_wait( boost::ref( first_handler ) );
    timer_call_back second_handler;
    second_timer.async_wait( boost::ref( second_handler ) );

    asio_mocks::current_time( first_expected_time );
    tools::run( queue );
    first_handler.check_called_without_error_at( first_expected_time );
    second_handler.check_not_called();

    asio_mocks::current_time( second_expected_time );
    tools::run( queue );
    first_handler.check_not_called();
    second_handler.check_called_without_error_at( second_expected_time );
}

BOOST_AUTO_TEST_CASE( dtor_invokes_cancel_cb )
{
    boost::asio::io_service queue;

    timer_call_back handler;

    {
        asio_mocks::timer timer( queue );
        BOOST_CHECK_EQUAL( timer.expires_from_now( boost::posix_time::seconds( 2 ) ), 0 );

        timer.async_wait( boost::ref( handler ) );

        tools::run( queue );
        handler.check_not_called();
    }

    tools::run( queue );
    handler.check_canceled();
}

BOOST_AUTO_TEST_CASE( cancel_a_single_timer_cb )
{
    boost::asio::io_service queue;

    timer_call_back handler;
    asio_mocks::timer timer( queue );
    BOOST_CHECK_EQUAL( timer.expires_from_now( boost::posix_time::seconds( 2 ) ), 0 );

    timer.async_wait( boost::ref( handler ) );

    tools::run( queue );
    handler.check_not_called();

    BOOST_CHECK_EQUAL( timer.cancel(), 1u );

    tools::run( queue );
    handler.check_canceled();
}

BOOST_AUTO_TEST_CASE( cancel_multiple_timer_cbs )
{
    boost::asio::io_service queue;

    timer_call_back handlerA;
    timer_call_back handlerB;
    asio_mocks::timer timer( queue );
    BOOST_CHECK_EQUAL( timer.expires_from_now( boost::posix_time::seconds( 2 ) ), 0 );

    timer.async_wait( boost::ref( handlerA ) );
    timer.async_wait( boost::ref( handlerB ) );

    tools::run( queue );
    handlerA.check_not_called();
    handlerB.check_not_called();

    BOOST_CHECK_EQUAL( timer.cancel(), 2u );

    tools::run( queue );
    handlerA.check_canceled();
    handlerB.check_canceled();
}

BOOST_AUTO_TEST_CASE( resetting_expiration_time_cancels_timer )
{
    boost::asio::io_service queue;

    timer_call_back handlerA;
    timer_call_back handlerB;
    asio_mocks::timer timer( queue );
    BOOST_CHECK_EQUAL( timer.expires_from_now( boost::posix_time::seconds( 2 ) ), 0 );

    timer.async_wait( boost::ref( handlerA ) );
    timer.async_wait( boost::ref( handlerB ) );

    tools::run( queue );
    handlerA.check_not_called();
    handlerB.check_not_called();

    BOOST_CHECK_EQUAL( timer.expires_from_now( boost::posix_time::seconds( 2 ) ), 2u );

    tools::run( queue );
    handlerA.check_canceled();
    handlerB.check_canceled();

    asio_mocks::current_time( boost::posix_time::time_from_string( "1970-01-01 00:00:02") );

    tools::run( queue );
    handlerA.check_not_called();
    handlerB.check_not_called();

    asio_mocks::current_time( boost::posix_time::time_from_string( "1970-01-01 00:00:03") );

    tools::run( queue );
    handlerA.check_not_called();
    handlerB.check_not_called();
}

BOOST_AUTO_TEST_CASE( advance_time_test )
{
    asio_mocks::reset_time();
    const boost::posix_time::ptime start_time = asio_mocks::current_time();
    const boost::posix_time::ptime t1 = start_time + boost::posix_time::seconds( 1 );
    const boost::posix_time::ptime t5 = start_time + boost::posix_time::seconds( 5 );
    const boost::posix_time::ptime t7 = start_time + boost::posix_time::seconds( 7 );

    boost::asio::io_service queue;

    asio_mocks::timer timerA( queue );
    asio_mocks::timer timerB( queue );
    asio_mocks::timer timerC( queue );
    asio_mocks::timer timerD( queue );

    BOOST_CHECK_EQUAL( start_time, asio_mocks::current_time() );

    timer_call_back handlerA;
    timer_call_back handlerB;
    timer_call_back handlerC;
    timer_call_back handlerD;

    BOOST_TEST_MESSAGE( "setup timers" );

    timerA.expires_at( t5 );
    timerA.async_wait( boost::ref( handlerA ) );
    timerB.expires_at( t1 );
    timerB.async_wait( boost::ref( handlerB ) );
    timerC.expires_at( t7 );
    timerC.async_wait( boost::ref( handlerC ) );
    timerD.expires_at( t5 );
    timerD.async_wait( boost::ref( handlerD ) );

    BOOST_REQUIRE_EQUAL( 1u, asio_mocks::advance_time() );
    BOOST_CHECK_EQUAL( t1, asio_mocks::current_time() );

    tools::run( queue );

    handlerA.check_not_called();
    handlerB.check_called_without_error_at( t1 );
    handlerC.check_not_called();
    handlerD.check_not_called();

    BOOST_REQUIRE_EQUAL( 2u, asio_mocks::advance_time() );
    BOOST_CHECK_EQUAL( t5, asio_mocks::current_time() );

    tools::run( queue );

    handlerA.check_called_without_error_at( t5 );
    handlerB.check_not_called();
    handlerC.check_not_called();
    handlerD.check_called_without_error_at( t5 );

    BOOST_REQUIRE_EQUAL( 1u, asio_mocks::advance_time() );
    BOOST_CHECK_EQUAL( t7, asio_mocks::current_time() );

    tools::run( queue );

    handlerA.check_not_called();
    handlerB.check_not_called();
    handlerC.check_called_without_error_at( t7 );
    handlerD.check_not_called();
}
