#ifndef SIOUX_SOURCE_JSON_HEURISTIC_H
#define SIOUX_SOURCE_JSON_HEURISTIC_H

#include <cstddef>
#include <vector>

namespace json {
    class array;

namespace details {

    /**
     * @brief provides a "resonable" heuristic for A*
     *
     * The implementation builds an index and provides the difference of the two array without 
     * some elements at the start of the arrays.
     *
     * For a A*, it's important to have a heuristic that estimates the remaining costs to the target
     * as high as possible to be efficent. On the other hand, the heuristic should not overestimate the
     * costs, or the result of the A* will not be optimal. 
     *
     * The heuristic looks at the size differences of the remainings of the arrays a and b. If the json 
     * encoding of the remaining of a is shorter than the json encoding of the remaining of b, there must
     * be at least one update or insert that contains informations at least as large as this difference.
     *
     * If the json encoding of the remaining of a is shorter, the minimum costs to update the remaining of a
     * to the remaining of b is the cost of at least one delete operation and this is 4 at maximum (code + index + 2 commas).
     */
    class heuristic
    {
    public:
        heuristic( const array& a, const array& b );

        /**
         * estimates consts for updating a from a_index to the end of a to become
         * b from b_index to the end.
         */
        std::size_t operator()( std::size_t a_index, std::size_t b_index ) const;
    private:
        typedef std::vector< std::size_t > index_t;
        index_t a_;
        index_t b_;

        static index_t build_index( const array& arr );
    };
}
}

#endif 

