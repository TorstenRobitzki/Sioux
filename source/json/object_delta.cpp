#include "json/delta.h"
#include "json/json.h"
#include "json/update_codes.h"

namespace json
{
    std::pair<bool, value> delta( const object& a, const object& b, std::size_t max_size )
    {
        array result;

        const std::vector<string> akeys = a.keys();
        const std::vector<string> bkeys = b.keys();

        std::vector<string>::const_iterator pa = akeys.begin(), pb = bkeys.begin();

        for ( ; (pa != akeys.end() || pb != bkeys.end()) && result.size() < max_size; ) 
        {
            if ( pb == bkeys.end() || pa != akeys.end() && *pa < *pb )
            {
                result.add( delete_at_operation() )
                      .add( *pa );

                ++pa;
            }
            else if ( pa == akeys.end() || pb != bkeys.end() && *pb < *pa )
            {
                result.add( insert_at_operation() )
                      .add( *pb )
                      .add( b.at( *pb ) );

                ++pb;
            }
            else
            {
                assert( *pa == *pb );
                const value b_element = b.at( *pb );

                std::pair<bool, value> edit_op = delta(a.at(*pa), b_element, max_size - result.size());

                // use edit, if possible and shorter
                if ( edit_op.first && edit_op.second.size() < b_element.size() )
                {
                    result.add( edit_at_operation() )
                          .add( *pa )
                          .add( edit_op.second );

                }
                else
                {
                    result.add( update_at_operation() )
                          .add( *pa)
                          .add( b_element );
                }

                ++pa;
                ++pb;
            }
        }

        return result.size() <= max_size
            ? std::make_pair(true, value(result))
            : std::make_pair(false, value(b));
    }
}
