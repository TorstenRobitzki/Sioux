#include "json/internal/heuristic.h"
#include "json/json.h"

namespace json {
namespace details {

    heuristic::index_t heuristic::build_index( const array& arr )
    {
        index_t result( 1u, 0 );

        for ( std::size_t i = arr.length(); i != 0; --i )
        {
            result.push_back( result.back() + arr.at( i - 1 ).size() + 1 );
        }

        // the first element is not trailed by a comma
        if ( result.size() != 1u )
            --result.back();

        return index_t( result.rbegin(), result.rend() );
    }

    heuristic::heuristic( const array& a, const array& b )
        : a_( build_index( a ) )
        , b_( build_index( b ) )
    {
        assert( a_.size() == a.length() +1 ); 
        assert( b_.size() == b.length() +1 ); 
    }

    std::size_t heuristic::operator()( std::size_t a_index, std::size_t b_index ) const
    {
        assert( a_index < a_.size() );
        assert( b_index < b_.size() );

        const std::size_t a = a_[ a_index ];
        const std::size_t b = b_[ b_index ];

        if ( a == b )
            return 0;

        static const std::size_t minmum_costs_for_a_single_delete_operations = 4u;

        return b > a ? b - a : minmum_costs_for_a_single_delete_operations;
    }

}
}
