#include "json/merge_operations.h"
#include "json/json.h"
#include "json/delta.h"


namespace json {
    namespace details {

        static number increment(const value& v)
        {
            return number(v.upcast<number>().to_int()+1);
        }

        bool merge_change_element( const array& last_update, int index, const value& b, array& result )
        {
            const value prev_op = last_update.empty() ? null() : last_update.at( 0 );
            bool merged = true;

            if ( prev_op == update_at_operation() )
            {
                // combine a previous update with this update to a range update
                array new_elements;
                new_elements.add( last_update.at( 2 ) ).add( b );

                result.add( update_range_operation() )
                      .add( last_update.at( 1 ) )
                      .add( number( index + 1 ) )
                      .add( new_elements );
            }
            else if ( prev_op == insert_at_operation() )
            {
                // combine a previous insert with this update to a range update
                array new_elements( last_update.at( 2 ) );
                new_elements.add( b );

                assert( index > 0 );

                result.add( update_range_operation() )
                      .add( number( index - 1 ) )
                      .add( number( index ) )
                      .add( new_elements );
            }
            else if ( prev_op == delete_at_operation() )
            {
                // combine a previous delete with this update to a range update
                result.add( update_range_operation() )
                      .add( number( index ) )
                      .add( number( index + 2 ) )
                      .add( array( b ) );
            }
            else if ( prev_op == delete_range_operation() )
            {
                // combine a previous range delete with this update to a range update
                result.add( update_range_operation() )
                      .add( last_update.at( 1 ) )
                      .add( increment( last_update.at( 2 ) ) )
                      .add( array( b ) );
            }
            else if ( prev_op == update_range_operation() )
            {
                array new_elements( last_update.at( 3 ).upcast< array >().copy() );
                new_elements.add( b );

                // combine a previous update with this update to a range update
                result.add( update_range_operation() )
                      .add( last_update.at( 1 ) )
                      .add( increment( last_update.at( 2 ) ) )
                      .add( new_elements );
            }
            else
            {
                merged = false;
            }

            return merged;
        }

    }
}
