#include "json/internal/operations.h"
#include "json/delta.h"
#include "tools/asstring.h"

namespace json
{
namespace operations {

    namespace {
        template < class ThisType >
        struct merge_result : visitor {
            boost::shared_ptr< update_operation > result;
            const ThisType* self;

            void dispatch( const ThisType& s, const update_operation& o )
            {
                self = &s;
                o.accept( *this );
            }
        };

    }

    //////////////////////////
    // class update_operation
    boost::shared_ptr< update_operation > update_operation::merge( const update_operation& other ) const
    {
        return boost::shared_ptr< update_operation >();
    }

    ///////////////////
    // class update_at
    update_at::update_at( int position, const value& new_value )
        : position_( position )
        , new_value_( new_value )
    {
    }

    int update_at::position() const
    {
        return position_;
    }

    const value& update_at::new_value() const
    {
        return new_value_;
    }

    void update_at::accept( visitor& v ) const
    {
        v.visit( *this );
    }

    void update_at::serialize( array& output ) const
    {
        output.add( update_at_operation() ).add( json::number( position_ ) ).add( new_value_ );
    }

    boost::shared_ptr< update_operation > update_at::merge( const update_operation& other ) const
    {
        struct : merge_result< update_at >
        {
            void visit( const update_at& other )
            {
                if ( self->position() + 1 != other.position() )
                    return;

                array combined;
                combined.add( self->new_value() ).add( other.new_value() );

                result.reset( new update_range( self->position(), self->position() + 2, combined ) );
            }

            void visit( const insert_at&  other )
            {
                if ( self->position() == other.position() )
                {
                    array combined;
                    combined.add( other.new_value() ).add( self->new_value() );

                    result.reset( new update_range( self->position(), self->position() + 1, combined ) );
                }
                else if ( self->position() + 1 == other.position() )
                {
                    array combined;
                    combined.add( self->new_value() ).add( other.new_value() );

                    result.reset( new update_range( self->position(), self->position() + 1, combined ) );
                }
            }

            void visit( const delete_at& other )
            {
                if ( self->position() == other.position() )
                {
                    result.reset( new delete_at( other ) );
                }
                else if ( self->position() + 1 == other.position() )
                {
                    result.reset( new update_range( self->position(), self->position() + 2, array( self->new_value() ) ) );
                }
            }

        } result;

        result.dispatch( *this, other );
        return result.result;
    }

    /////////////////
    // class edit_at
    edit_at::edit_at( int position, const value& update_instructions )
        : position_( position )
        , update_instructions_( update_instructions )
    {
    }

    void edit_at::accept( visitor& v ) const
    {
        v.visit( *this );
    }

    void edit_at::serialize( array& output ) const
    {
        output.add( edit_at_operation() ).add( json::number( position_ ) ).add( update_instructions_ );
    }

    ///////////////////
    // class delete_at
    delete_at::delete_at( int position )
        : position_( position )
    {
    }

    int delete_at::position() const
    {
        return position_;
    }

    void delete_at::accept( visitor& v ) const
    {
        v.visit( *this );
    }

    void delete_at::serialize( array& output ) const
    {
        output.add( delete_at_operation() ).add( json::number( position_ ) );
    }

    boost::shared_ptr< update_operation > delete_at::merge( const update_operation& other ) const
    {
        struct : merge_result< delete_at >
        {
            void visit( const update_at& other )
            {
                if ( self->position() == other.position() )
                {
                    result.reset( new update_range( self->position(), self->position() + 2, json::array( other.new_value() ) ) );
                }
                else if ( self->position() == other.position() + 1 )
                {
                    result.reset( new update_range( other.position(), other.position() + 2, json::array( other.new_value() ) ) );
                }
            }

            void visit( const insert_at&  other )
            {
                if ( self->position() == other.position() )
                {
                    result.reset( new update_at( self->position(), other.new_value() ) );
                }
            }

            void visit( const delete_at& other )
            {
                if ( self->position() == other.position() )
                {
                    result.reset( new delete_range( self->position(), self->position() + 2 ) );
                }
                else if ( self->position() == other.position() + 1 )
                {
                    result.reset( new delete_range( other.position(), other.position() + 2 ) );
                }
            }

        } result;

        result.dispatch( *this, other );
        return result.result;
    }

    ///////////////////
    // class insert_at
    insert_at::insert_at( int position, const value& new_value )
        : position_( position )
        , new_value_( new_value )
    {
    }

    int insert_at::position() const
    {
        return position_;
    }

    const value& insert_at::new_value() const
    {
        return new_value_;
    }

    void insert_at::accept( visitor& v ) const
    {
        v.visit( *this );
    }

    void insert_at::serialize( array& output ) const
    {
        output.add( insert_at_operation() ).add( json::number( position_ ) ).add( new_value_ );
    }

    boost::shared_ptr< update_operation > insert_at::merge( const update_operation& other ) const
    {
        struct : merge_result< insert_at >
        {
            void visit( const update_at& other )
            {
                if ( self->position() == other.position() + 1 )
                {
                    result.reset( new update_range( other.position(), other.position() + 1,
                        json::array( other.new_value(), self->new_value() ) ) );
                }
                else if ( self->position() + 1 == other.position()  )
                {
                    result.reset( new update_range( self->position(), self->position() + 1,
                        json::array( self->new_value(), other.new_value() ) ) );
                }
                else if ( self->position() == other.position() )
                {
                    result.reset( new insert_at( self->position(), other.new_value() ) );
                }
            }

            void visit( const insert_at&  other )
            {
                if ( self->position() == other.position() )
                {
                    result.reset( new update_range( self->position(), self->position(), array( other.new_value(), self->new_value() ) ) );
                }
                else if ( self->position() + 1 == other.position() )
                {
                    result.reset( new update_range( self->position(), self->position(), array( self->new_value(), other.new_value() ) ) );
                }
            }

            void visit( const delete_at& other )
            {
                if ( self->position() == other.position() + 1 )
                {
                    result.reset( new update_at( other.position(), self->new_value() ) );
                }
            }

        } result;

        result.dispatch( *this, other );
        return result.result;
    }

    //////////////////////
    // class delete_range
    delete_range::delete_range( int from, int to )
        : from_( from )
        , to_( to )
    {
        if ( to < from )
            throw std::invalid_argument( "from must be smaller than to delete_range(" + tools::as_string( from )
                + ", " + tools::as_string( to ) + ")" );
    }

    int delete_range::from() const
    {
        return from_;
    }

    int delete_range::to() const
    {
        return to_;
    }

    void delete_range::accept( visitor& v ) const
    {
        v.visit( *this );
    }

    void delete_range::serialize( array& output ) const
    {
        output.add( delete_range_operation() ).add( json::number( from_ ) ).add( json::number( to_ ) );
    }

    boost::shared_ptr< update_operation > delete_range::merge( const update_operation& other ) const
    {
        struct : merge_result< delete_range >
        {
            void visit( const update_at& other )
            {
                if ( self->from() == other.position() )
                {
                    result.reset( new update_range( self->from(), self->to() + 1, array( other.new_value() ) ) );
                }
                else if ( self->from() == other.position() + 1 )
                {
                    result.reset( new update_range( other.position(), self->to(), array( other.new_value() ) ) );
                }
            }

            void visit( const insert_at& other )
            {
                if ( self->from() == other.position() )
                {
                    result.reset( new update_range( self->from(), self->to(), array( other.new_value() ) ) );
                }
            }

            void visit( const delete_at& other )
            {
                if ( self->from() == other.position()  )
                {
                    result.reset( new delete_range( self->from(), self->to() + 1 ) );
                }
                else if ( self->from() == other.position() + 1 )
                {
                    result.reset( new delete_range( other.position(), self->to() ) );
                }
            }

        } result;

        result.dispatch( *this, other );
        return result.result;
    }

    //////////////////////
    // class update_range
    update_range::update_range( int from, int to, const array& values )
        : from_( from )
        , to_( to )
        , new_values_( values )
    {
        if ( to < from )
            throw std::invalid_argument( "from must be smaller than to update_range(" + tools::as_string( from )
                + ", " + tools::as_string( to ) + ")" );
    }

    int update_range::from() const
    {
        return from_;
    }

    int update_range::to() const
    {
        return to_;
    }

    const array& update_range::new_values() const
    {
        return new_values_;
    }

    void update_range::accept( visitor& v ) const
    {
        v.visit( *this );
    }

    void update_range::serialize( array& output ) const
    {
        output.add( update_range_operation() ).add( json::number( from_ ) ).add( json::number( to_ ) ).add( new_values_ );
    }

    boost::shared_ptr< update_operation > update_range::merge( const update_operation& other ) const
    {
        struct : merge_result< update_range >
        {
            void visit( const update_at& other )
            {
                if ( self->from() == other.position() + 1 )
                {
                    array new_values = self->new_values().copy();
                    new_values.insert( 0, other.new_value() );

                    result.reset( new update_range( other.position(), self->to(), new_values ) );
                }
                else
                {
                    const int to_in_result = self->from() + self->new_values().length();

                    if ( other.position() == to_in_result )
                    {
                        array new_values = self->new_values().copy();
                        new_values.add( other.new_value() );

                        result.reset( new update_range( self->from(), self->to() + 1, new_values ) );
                    }
                    else if ( other.position() >= self->from() && other.position() < to_in_result )
                    {
                        array new_values = self->new_values().copy();
                        new_values.at( other.position() - other.position() ) = other.new_value();

                        result.reset( new update_range( self->from(), self->to(), new_values ) );
                    }
                }
            }

            void visit( const insert_at& other )
            {
                const int to_in_result = self->from() + self->new_values().length();

                if ( other.position() >= self->from() && other.position() <= to_in_result )
                {
                    array new_values = self->new_values().copy();
                    new_values.insert( other.position() - self->from(), other.new_value() );

                    result.reset( new update_range( self->from(), self->to(), new_values ) );
                }
            }

            void visit( const delete_at& other )
            {
                if ( self->from() == other.position() + 1 )
                {
                    result.reset( new update_range( other.position(), self->to(), self->new_values() ) );
                }
                else
                {
                    const int to_in_result = self->from() + self->new_values().length();

                    if ( other.position() == to_in_result )
                    {
                        result.reset( new update_range( self->from(), self->to() + 1, self->new_values() ) );
                    }
                    else if ( other.position() >= self->from() && other.position() < to_in_result )
                    {
                        array new_values = self->new_values().copy();
                        new_values.erase( other.position() - other.position(), 1 );

                        result.reset( new update_range( self->from(), self->to(), new_values ) );
                    }
                }
            }

        } result;

        result.dispatch( *this, other );
        return result.result;
    }

    array& operator<<( array& output, const update_operation& op )
    {
        op.serialize( output );
        return output;
    }

    //////////////////
    // class visitor
    void visitor::visit( const update_at& )
    {
    }

    void visitor::visit( const edit_at& )
    {
    }

    void visitor::visit( const delete_at& )
    {
    }

    void visitor::visit( const insert_at& )
    {
    }

    void visitor::visit( const delete_range& )
    {
    }

    void visitor::visit( const update_range& )
    {
    }


}
}
