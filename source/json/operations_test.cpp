#include <boost/test/unit_test.hpp>
#include "json/internal/operations.h"
#include "json/delta.h"
#include "tools/dynamic_type.h"
#include "tools/asstring.h"
#include <typeinfo>

namespace
{

    class test_visitor : public json::operations::visitor
    {
    public:
        class null_type {};

        test_visitor()
            : i_( 0 )
            , type_( typeid( null_type ) )
        {
        }

        template < class T >
        bool same_instance( const T& instance )
        {
            return i_ == &instance;
        }

        template < class T >
        bool same_type( const T& instance )
        {
            return type_ == tools::dynamic_type( typeid( T ) );
        }
    private:
        void visit( const json::operations::update_at& v ) { set( v ); }
        void visit( const json::operations::edit_at& v ) { set( v ); }
        void visit( const json::operations::delete_at& v ) { set( v ); }
        void visit( const json::operations::insert_at& v ) { set( v ); }
        void visit( const json::operations::delete_range& v ) { set( v ); }
        void visit( const json::operations::update_range& v ) { set( v ); }

        template < class T >
        void set( const T& v )
        {
            i_ = &v;
            type_ = tools::dynamic_type( typeid( T ) );
        }

        const void* i_;
        tools::dynamic_type type_;
    };

    json::array to_array( const json::operations::update_operation& op )
    {
        json::array out;
        out << op;

        return out;
    }

    boost::shared_ptr< json::operations::update_operation > merge( const json::operations::update_operation& op1,
        const json::operations::update_operation& op2 )
    {
        return op1.merge( op2 );
    }

    void check_equal_effect( const json::operations::update_operation& op1, const json::operations::update_operation& op2,
        const json::operations::update_operation& combined )
    {
        json::array test_data;
        for ( int i = 0; i != 30; ++i )
            test_data.add( json::number( i ) );

        json::value single_result       = update( test_data, to_array( op1 ) );
        single_result                   = update( single_result, to_array( op2 ) );

        const json::value merged_result = update( test_data, to_array( combined ) );

        BOOST_CHECK_EQUAL( single_result, merged_result );
    }

    void check_equal_effect( const json::operations::update_operation& op1, const json::operations::update_operation& op2 )
    {
        boost::shared_ptr< json::operations::update_operation > merged = op1.merge( op2 );

        BOOST_REQUIRE( merged.get() );
        check_equal_effect( op1, op2, *merged );
    }

    void check_size( const json::operations::update_operation& op )
    {
        json::array out;
        op.serialize( out );

        BOOST_CHECK_EQUAL( out.size(), op.size() );
    }
}

BOOST_AUTO_TEST_CASE( update_operation_visiting )
{
    test_visitor v;
    json::operations::update_at update( 5, json::false_val() );

    v.apply( update );

    BOOST_CHECK( v.same_instance( update ) );
    BOOST_CHECK( v.same_type( update ) );
}

BOOST_AUTO_TEST_CASE( serialize_update_operation )
{
    json::array output;

    json::operations::update_at update1( 5, json::false_val() );
    json::operations::update_at update2( 17, json::number( 12 ) );

    output << update1;

    BOOST_CHECK_EQUAL( json::parse( "[ 1, 5, false ]" ), output );

    output << update2;

    BOOST_CHECK_EQUAL( json::parse( "[ 1, 5, false, 1, 17, 12 ]" ), output );
}

BOOST_AUTO_TEST_CASE( merge_update_with_update )
{
    json::operations::update_at update1( 5, json::false_val() );
    json::operations::update_at update2( 6, json::number( 12 ) );
    json::operations::update_at update3( 8, json::null() );

    check_equal_effect( update1, update2 );

    BOOST_CHECK( merge( update1, update3 ).get() == 0 );
    BOOST_CHECK( merge( update2, update3 ).get() == 0 );
}

BOOST_AUTO_TEST_CASE( merge_update_with_insert )
{
    json::operations::update_at update1( 5, json::false_val() );
    json::operations::insert_at insert1( 5, json::number( 12 ) );
    json::operations::insert_at insert2( 6, json::null() );
    json::operations::insert_at insert3( 7, json::null() );

    check_equal_effect( update1, insert1 );
    check_equal_effect( update1, insert2 );

    BOOST_CHECK( merge( update1, insert3 ).get() == 0 );
}

BOOST_AUTO_TEST_CASE( merge_update_with_delete )
{
    json::operations::update_at update1( 5, json::false_val() );
    json::operations::delete_at delete1( 5 );
    json::operations::delete_at delete2( 6 );
    json::operations::delete_at delete3( 7 );

    check_equal_effect( update1, delete1 );
    check_equal_effect( update1, delete2 );

    BOOST_CHECK( merge( update1, delete3 ).get() == 0 );
}

BOOST_AUTO_TEST_CASE( edit_operation_visiting )
{
    test_visitor v;
    json::operations::edit_at edit( 4, json::string("hallo") );

    v.apply( edit );

    BOOST_CHECK( v.same_instance( edit ) );
    BOOST_CHECK( v.same_type( edit ) );
}

BOOST_AUTO_TEST_CASE( serialize_edit_operation )
{
    json::array output;

    json::operations::edit_at edit1( 5, json::false_val() );
    json::operations::edit_at edit2( 17, json::number( 12 ) );

    output << edit1;

    BOOST_CHECK_EQUAL( json::parse( "[ 6, 5, false ]" ), output );

    output << edit2;

    BOOST_CHECK_EQUAL( json::parse( "[ 6, 5, false, 6, 17, 12 ]" ), output );
}

BOOST_AUTO_TEST_CASE( delete_operation_visiting )
{
    test_visitor v;
    json::operations::delete_at del( 4 );

    v.apply( del );

    BOOST_CHECK( v.same_instance( del ) );
    BOOST_CHECK( v.same_type( del ) );
}

BOOST_AUTO_TEST_CASE( serialize_delete_operation )
{
    json::array output;

    json::operations::delete_at del1( 5 );
    json::operations::delete_at del2( 17 );

    output << del1;

    BOOST_CHECK_EQUAL( json::parse( "[ 2, 5 ]" ), output );

    output << del2;

    BOOST_CHECK_EQUAL( json::parse( "[ 2, 5, 2, 17 ]" ), output );
}

BOOST_AUTO_TEST_CASE( merge_delete_with_update )
{
    json::operations::delete_at delete1( 5 );
    json::operations::update_at update1( 5, json::true_val() );
    json::operations::update_at update2( 4, json::null() );
    json::operations::update_at update3( 1, json::string( "asd" ) );

    check_equal_effect( delete1, update1 );
    check_equal_effect( delete1, update2 );

    BOOST_CHECK( merge( delete1, update3 ).get() == 0 );
}

BOOST_AUTO_TEST_CASE( merge_delete_with_insert )
{
    json::operations::delete_at delete1( 5 );
    json::operations::insert_at insert1( 5, json::true_val() );
    json::operations::insert_at insert2( 6, json::string( "asd" ) );

    check_equal_effect( delete1, insert1 );

    BOOST_CHECK( merge( delete1, insert2 ).get() == 0 );
}

BOOST_AUTO_TEST_CASE( merge_delete_with_delete )
{
    json::operations::delete_at delete1( 5 );
    json::operations::delete_at delete2( 5 );
    json::operations::delete_at delete3( 4 );
    json::operations::delete_at delete4( 1 );

    check_equal_effect( delete1, delete2 );
    check_equal_effect( delete1, delete3 );

    BOOST_CHECK( merge( delete1, delete4 ).get() == 0 );
    BOOST_CHECK( merge( delete3, delete2 ).get() == 0 );
}

BOOST_AUTO_TEST_CASE( insert_operation_visiting )
{
    test_visitor v;
    json::operations::insert_at insert( 4, json::string("hallo") );

    v.apply( insert );

    BOOST_CHECK( v.same_instance( insert ) );
    BOOST_CHECK( v.same_type( insert ) );
}

BOOST_AUTO_TEST_CASE( serialize_insert_operation )
{
    json::array output;

    json::operations::insert_at insert1( 5, json::false_val() );
    json::operations::insert_at insert2( 17, json::number( 12 ) );

    output << insert1;

    BOOST_CHECK_EQUAL( json::parse( "[ 3, 5, false ]" ), output );

    output << insert2;

    BOOST_CHECK_EQUAL( json::parse( "[ 3, 5, false, 3, 17, 12 ]" ), output );
}

BOOST_AUTO_TEST_CASE( merge_insert_with_update )
{
    json::operations::insert_at insert( 3, json::string( "Hello" ) );
    json::operations::update_at update1( 2, json::number( 2 ) );
    json::operations::update_at update2( 3, json::number( 3 ) );
    json::operations::update_at update3( 4, json::number( 4 ) );
    json::operations::update_at update4( 5, json::number( 4 ) );

    check_equal_effect( insert, update1 );
    check_equal_effect( insert, update2 );
    check_equal_effect( insert, update3 );

    BOOST_CHECK( merge( insert, update4 ).get() == 0 );
}

BOOST_AUTO_TEST_CASE( merge_insert_with_insert )
{
    json::operations::insert_at insert1( 3, json::string( "Hello" ) );
    json::operations::insert_at insert2( 3, json::number( 2 ) );
    json::operations::insert_at insert3( 4, json::number( 3 ) );
    json::operations::insert_at insert4( 5, json::number( 4 ) );

    check_equal_effect( insert1, insert2 );
    check_equal_effect( insert1, insert3 );

    BOOST_CHECK( merge( insert1, insert4 ).get() == 0 );
}

BOOST_AUTO_TEST_CASE( merge_insert_with_delete )
{
    json::operations::insert_at insert( 3, json::string( "Hello" ) );
    json::operations::delete_at delete1( 3 );
    json::operations::delete_at delete2( 2 );
    json::operations::delete_at delete3( 5 );

    check_equal_effect( insert, delete2 );

    BOOST_CHECK( merge( insert, delete1 ).get() == 0 );
    BOOST_CHECK( merge( insert, delete3 ).get() == 0 );
}

BOOST_AUTO_TEST_CASE( range_delete_operation_visiting )
{
    test_visitor v;
    json::operations::delete_range del( 4, 5 );

    v.apply( del );

    BOOST_CHECK( v.same_instance( del ) );
    BOOST_CHECK( v.same_type( del ) );
}

BOOST_AUTO_TEST_CASE( serialize_range_delete_operation )
{
    json::array output;

    json::operations::delete_range del1( 5, 8 );
    json::operations::delete_range del2( 17, 19 );

    output << del1;

    BOOST_CHECK_EQUAL( json::parse( "[ 4, 5, 8 ]" ), output );

    output << del2;

    BOOST_CHECK_EQUAL( json::parse( "[ 4, 5, 8, 4, 17, 19 ]" ), output );
}

BOOST_AUTO_TEST_CASE( range_delete_checks_ctor_arguments )
{
    BOOST_CHECK_THROW( json::operations::delete_range(10,1), std::logic_error );
}

BOOST_AUTO_TEST_CASE( merge_delete_range_with_delete )
{
    json::operations::delete_range delete1( 3, 7 );
    json::operations::delete_at delete2( 2 );
    json::operations::delete_at delete3( 3 );
    json::operations::delete_at delete4( 4 );

    check_equal_effect( delete1, delete2 );
    check_equal_effect( delete1, delete3 );

    BOOST_CHECK( merge( delete1, delete4 ).get() == 0 );
}

BOOST_AUTO_TEST_CASE( merge_delete_range_with_update )
{
    json::operations::delete_range delete1( 3, 7 );
    json::operations::update_at update1( 2, json::number( 7 ) );
    json::operations::update_at update2( 3, json::number( 12 ) );
    json::operations::update_at update3( 4, json::number( 42 ) );

    check_equal_effect( delete1, update1 );
    check_equal_effect( delete1, update2 );

    BOOST_CHECK( merge( delete1, update3 ).get() == 0 );
}

BOOST_AUTO_TEST_CASE( merge_delete_range_with_insert )
{
    json::operations::delete_range delete1( 3, 7 );
    json::operations::insert_at insert1( 3, json::number( 7 ) );
    json::operations::insert_at insert2( 2, json::number( 12 ) );
    json::operations::insert_at insert3( 4, json::number( 42 ) );

    check_equal_effect( delete1, insert1 );

    BOOST_CHECK( merge( delete1, insert2 ).get() == 0 );
    BOOST_CHECK( merge( delete1, insert3 ).get() == 0 );
}

BOOST_AUTO_TEST_CASE( range_update_operation_visiting )
{
    test_visitor v;
    json::operations::update_range update( 4, 5, json::array() );

    v.apply( update );

    BOOST_CHECK( v.same_instance( update ) );
    BOOST_CHECK( v.same_type( update ) );
}

BOOST_AUTO_TEST_CASE( serialize_range_update_operation )
{
    json::array output;
    json::array a1 = json::parse( "[ null, false, \"abc\" ]" ).upcast< json::array >();
    json::array a2 = json::parse( "[ 1,2,3 ]" ).upcast< json::array >();

    json::operations::update_range update1( 5, 8, a1 );
    json::operations::update_range update2( 17, 19, a2 );

    output << update1;

    BOOST_CHECK_EQUAL( json::parse( "[ 5, 5, 8, [ null, false, \"abc\" ] ]" ), output );

    output << update2;

    BOOST_CHECK_EQUAL( json::parse( "[ 5, 5, 8, [ null, false, \"abc\" ], 5, 17, 19, [ 1,2,3 ] ]" ), output );
}

BOOST_AUTO_TEST_CASE( range_update_checks_ctor_arguments )
{
    BOOST_CHECK_THROW( json::operations::update_range( 10, 1, json::array()), std::logic_error );
}

// 1, 2, 3, 4, 5, 6, 7, 8, 9 initial data
// 1, 2, 1, null, 7, 8, 9    after update1
BOOST_AUTO_TEST_CASE( merge_update_range_with_update )
{
    json::operations::update_range update1( 3, 7, json::array( json::number( 1 ), json::null() ) );
    json::operations::update_at update2( 2, json::number( 7 ) );
    json::operations::update_at update3( 3, json::number( 12 ) );
    json::operations::update_at update4( 5, json::number( 42 ) );
    json::operations::update_at update5( 0, json::number( 99 ) );

    check_equal_effect( update1, update2 );
    check_equal_effect( update1, update3 );
    check_equal_effect( update1, update4 );

    BOOST_CHECK( merge( update1, update5 ).get() == 0 );
}

BOOST_AUTO_TEST_CASE( merge_update_range_with_insert )
{
    json::operations::update_range update1( 3, 7, json::array( json::number( 1 ), json::null() ) );
    json::operations::insert_at insert1( 3, json::number( 7 ) );
    json::operations::insert_at insert2( 4, json::number( 12 ) );
    json::operations::insert_at insert3( 5, json::number( 42 ) );
    json::operations::insert_at insert4( 0, json::number( 99 ) );
    json::operations::insert_at insert5( 2, json::number( 99 ) );

    check_equal_effect( update1, insert1 );
    check_equal_effect( update1, insert2 );
    check_equal_effect( update1, insert3 );

    BOOST_CHECK( merge( update1, insert4 ).get() == 0 );
    BOOST_CHECK( merge( update1, insert5 ).get() == 0 );
}

BOOST_AUTO_TEST_CASE( merge_update_range_with_delete )
{
    json::operations::update_range update1( 3, 7, json::array( json::number( 1 ), json::null() ) );
    json::operations::delete_at delete1( 2 );
    json::operations::delete_at delete2( 3 );
    json::operations::delete_at delete3( 5 );
    json::operations::delete_at delete4( 1 );
    json::operations::delete_at delete5( 6 );

    check_equal_effect( update1, delete1 );
    check_equal_effect( update1, delete2 );
    check_equal_effect( update1, delete3 );

    BOOST_CHECK( merge( update1, delete4 ).get() == 0 );
    BOOST_CHECK( merge( update1, delete5 ).get() == 0 );
}

BOOST_AUTO_TEST_CASE( update_at_size )
{
    check_size( json::operations::update_at( 1, json::string( "asdasd" ) ) );
    check_size( json::operations::update_at( 12, json::null() ) );
    check_size( json::operations::update_at( 999, json::true_val() ) );
}

BOOST_AUTO_TEST_CASE( edit_at_size )
{
    check_size( json::operations::edit_at( 0, json::array( json::number(12), json::string( "asdasdasd" ) ) ) );
    check_size( json::operations::edit_at( 14, json::array( json::number(1), json::false_val() ) ) );
}

BOOST_AUTO_TEST_CASE( delete_at_size )
{
    check_size( json::operations::delete_at( 0 ) );
    check_size( json::operations::delete_at( 7 ) );
    check_size( json::operations::delete_at( 1000 ) );
}

BOOST_AUTO_TEST_CASE( insert_at_size )
{
    check_size( json::operations::insert_at( 0, json::null() ) );
    check_size( json::operations::insert_at( 14, json::string( "asdasdasda" ) ) );
    check_size( json::operations::insert_at( 99, json::number( 99 ) ) );
}

BOOST_AUTO_TEST_CASE( update_range_size )
{
    check_size( json::operations::update_range( 0, 0, json::array( json::number( 12 ), json::null() ) ) );
    check_size( json::operations::update_range( 7, 44, json::array() ) );
    check_size( json::operations::update_range( 100, 999, json::array( json::string( "Es war einmal ein Baer..." ) ) ) );
}

BOOST_AUTO_TEST_CASE( delete_range_size )
{
    check_size( json::operations::delete_range( 1, 2 ) );
    check_size( json::operations::delete_range( 12, 99 ) );
    check_size( json::operations::delete_range( 44, 100 ) );
}


