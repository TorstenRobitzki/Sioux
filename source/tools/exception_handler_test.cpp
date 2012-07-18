// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "tools/exception_handler.h"
#include <boost/test/unit_test.hpp>
#include <sstream>

BOOST_AUTO_TEST_CASE( exception_is_rethrown_from_handler )
{
    try
    {
        throw std::string( "42" );
    }
    catch ( ... )
    {
        std::ostringstream out;
        BOOST_CHECK_THROW( tools::exception_text( out, tools::rethrow ), std::string );
    }
}

BOOST_AUTO_TEST_CASE( exception_is_not_rethrown_from_handler )
{
    try
    {
        throw std::string( "42" );
    }
    catch ( ... )
    {
        std::ostringstream out;
        tools::exception_text( out, tools::do_not_rethrow );
    }
}

#define TEST_EXCEPTION_HANDLER_WITH( a, s ) \
    do { try { \
        a; \
    } \
    catch ( ... ) \
    { \
        std::ostringstream out; \
        tools::exception_text( out, tools::do_not_rethrow ); \
        BOOST_CHECK_EQUAL( s, out.str() ); \
    } } while ( 0 )

namespace {

    class std_exception : public std::exception
    {
        virtual const char* what() const throw()
        {
            return "std_exception";
        }
    };
}

BOOST_AUTO_TEST_CASE( std_exception_is_handled )
{
    TEST_EXCEPTION_HANDLER_WITH( throw std_exception(),  "std::exception : \"std_exception\"" );
}

BOOST_AUTO_TEST_CASE( runtime_error_is_handled )
{
    TEST_EXCEPTION_HANDLER_WITH( throw std::runtime_error( "42" ), "std::runtime_error : \"42\"" );
}

BOOST_AUTO_TEST_CASE( range_error_is_handled )
{
    TEST_EXCEPTION_HANDLER_WITH( throw std::range_error( "42" ), "std::range_error : \"42\"" );
}

BOOST_AUTO_TEST_CASE( overflow_error_is_handled )
{
    TEST_EXCEPTION_HANDLER_WITH( throw std::overflow_error( "42" ), "std::overflow_error : \"42\"" );
}

BOOST_AUTO_TEST_CASE( underflow_error_is_handled )
{
    TEST_EXCEPTION_HANDLER_WITH( throw std::underflow_error( "42" ), "std::underflow_error : \"42\"" );
}

BOOST_AUTO_TEST_CASE( logic_error_is_handled )
{
    TEST_EXCEPTION_HANDLER_WITH( throw std::logic_error( "42" ), "std::logic_error : \"42\"" );
}

BOOST_AUTO_TEST_CASE( domain_error_is_handled )
{
    TEST_EXCEPTION_HANDLER_WITH( throw std::domain_error( "42" ), "std::domain_error : \"42\"" );
}

BOOST_AUTO_TEST_CASE( invalid_argument_is_handled )
{
    TEST_EXCEPTION_HANDLER_WITH( throw std::invalid_argument( "42" ), "std::invalid_argument : \"42\"" );
}

BOOST_AUTO_TEST_CASE( length_error_is_handled )
{
    TEST_EXCEPTION_HANDLER_WITH( throw std::length_error( "42" ), "std::length_error : \"42\"" );
}

BOOST_AUTO_TEST_CASE( out_of_range_is_handled )
{
    TEST_EXCEPTION_HANDLER_WITH( throw std::out_of_range( "42" ), "std::out_of_range : \"42\"" );
}

BOOST_AUTO_TEST_CASE( unknown_excpetion_is_handled )
{
    struct {} rumpelstilzchen;
    TEST_EXCEPTION_HANDLER_WITH( throw rumpelstilzchen, "unknown exception" );
}

BOOST_AUTO_TEST_CASE( integer_exception_is_handled )
{
    TEST_EXCEPTION_HANDLER_WITH( throw 42, "integer exception : \"42\"" );
}

BOOST_AUTO_TEST_CASE( char_ptr_exception_is_handled )
{
    TEST_EXCEPTION_HANDLER_WITH( throw "Hallo", "c string exception : \"Hallo\"" );
}

BOOST_AUTO_TEST_CASE( std_string_exception_is_handled )
{
    TEST_EXCEPTION_HANDLER_WITH( throw std::string( "Hallo" ), "std string exception : \"Hallo\"" );
}

#undef TEST_EXCEPTION_HANDLER_WITH

BOOST_AUTO_TEST_CASE( exception_text_test )
{
    try
    {
        throw 42;
    }
    catch ( ... )
    {
        BOOST_CHECK_EQUAL( tools::exception_text(), "integer exception : \"42\"" );
    }
}


