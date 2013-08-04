// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "json/delta.h"
#include "json/json.h"
#include "json/merge_operations.h"
#include "tools/asstring.h"
#include <stdexcept>
#include <set>

namespace json {


    const number& update_at_operation()
    {
        static const number result( update_at );
        return result;
    }

    const number& delete_at_operation()
    {
        static const number result( delete_at );
        return result;
    }

    const number& insert_at_operation()
    {
        static const number result( insert_at );
        return result;
    }

    const number& delete_range_operation()
    {
        static const number result( delete_range );
        return result;
    }

    const number& update_range_operation()
    {
        static const number result( update_range );
        return result;
    }

    const number& edit_at_operation()
    {
        static const number result( edit_at );
        return result;
    }


    namespace {
        struct vertex;

        typedef std::set<vertex> vertex_list_t;

        struct v_compare : std::binary_function<bool, vertex_list_t::const_iterator, vertex_list_t::const_iterator>
        {
            bool operator()(vertex_list_t::const_iterator lhs, vertex_list_t::const_iterator rhs) const;
        };

        typedef std::multiset< vertex_list_t::const_iterator, v_compare > cost_list_t;

        struct vertex
        {
            // the total length of the modified array
            int                             length;
            // all elements before this index are equal in the modified array and in the target array
            int                             index;

            // if array is not empty, previous must point to a valid vertex and when vertex points to
            // a valid vertex, the operation must not be empty
            array                           operation;
            vertex_list_t::const_iterator   previous;

            std::size_t                     total_costs;
            std::size_t                     costs;

            // order by identity
            bool operator<( const vertex& rhs ) const
            {
                return length < rhs.length
                    || length == rhs.length && index < rhs.index
                    || length == rhs.length && index && rhs.index && costs < rhs.costs;
            }
        };

        // order by costs
        bool v_compare::operator()(vertex_list_t::const_iterator lhs, vertex_list_t::const_iterator rhs) const
        {
            return lhs->total_costs < rhs->total_costs;
        }

        // this function should never ever overestimate the costs of updating a to be equal to b
        std::size_t heuristic(const array& a, const array& b, int a_index, int b_index)
        {
            assert( a_index <= a.length() );
            assert( b_index <= b.length() );

            const int a_length = a.length() - a_index;
            const int b_length = b.length() - b_index;

            // applying an array delete
            if ( a_length > b_length )
                return 2;

            // array - insert possible
            if ( a_length < b_length )
                return ( b_length - a_length ) * 2;

            return 0;
        }

        std::size_t heuristic(const array& a, const array& b)
        {
            return heuristic(a, b, 0, 0);
        }

        void add_open( vertex v, vertex_list_t& vlist, cost_list_t& clist, std::size_t heuristic, std::size_t max_size )
        {
            if ( v.previous != vlist.end() )
            {
                v.costs = v.previous->costs + v.operation.size() -1;
            }
            else
            {
                v.costs = 1;
            }

            v.total_costs = v.costs + heuristic;

            if ( v.total_costs > max_size )
                return;

            vertex_list_t::iterator pos = vlist.find(v);

            if ( pos != vlist.end() )
            {
                if ( v.total_costs < pos->total_costs )
                {
                    pos = vlist.insert(v).first;
                    clist.insert(pos);
                }
            }
            else 
            {
                pos = vlist.insert(vlist.begin(), v);
                clist.insert(pos);
            }
        }

        array assemble_result(vertex_list_t::const_iterator goal, const vertex_list_t& vertices)
        {
            array result;

            for (vertex_list_t::const_iterator i = goal ; i != vertices.end(); i = i->previous )
                result = i->operation + result;

            assert(result.size() == goal->total_costs);

            return result;
        }

        number increment(const value& v)
        {
            return number(v.upcast<number>().to_int()+1);
        }

        vertex change_element( vertex_list_t::const_iterator ptr, int index, const value& a, const value& b,
            bool merge_with_last_operation )
        {
            assert(a != b);

            array                           result;
            vertex_list_t::const_iterator   prev        = ptr->previous;
            const array                     last_update = ptr->operation;

            if ( !merge_with_last_operation || !details::merge_change_element( last_update, index, b, result ) )
            {
                result.add( update_at_operation() ).add( number( index ) ).add( b );
                prev = ptr;
            }

            assert( !result.empty() );
            const vertex newv  = { ptr->length, index+1, result, prev };

            return newv;
        }

        std::pair< vertex, bool > edit_element( vertex_list_t::const_iterator ptr, int index, const value& a, const value& b )
        {
            // limit the update to 2 * b.size(); because an update of a to b would for sure be cheaper
            const std::pair< bool, value > edit_operation = delta( a, b, 2 * b.size() );

            if ( edit_operation.first )
            {
                array result;
                result.add( edit_at_operation() )
                      .add( number( index ) )
                      .add( edit_operation.second );

                const vertex newv  = { ptr->length, index+1, result, ptr };

                return std::make_pair( newv, true );
            }

            return std::make_pair( vertex(), false );
        }

        vertex insert_element( vertex_list_t::const_iterator ptr, int index, const value& b, const value& prev_op )
        {
            array result;
            vertex_list_t::const_iterator prev = ptr->previous;

            const array last_update = ptr->operation;

            // a combination of a delete/range delete with an insert to an range update is 
            // not implemented, because an update should be considered too

            if ( prev_op == update_at_operation() )
            {
            	array new_elements( last_update.at( 2 ) );
                new_elements.add( b );

                result.add( update_range_operation() )
                      .add( last_update.at( 1 ) )
                      .add( number( index ) )
                      .add( new_elements );
            }
            else if ( prev_op == insert_at_operation() )
            {
                array new_elements( last_update.at( 2 ) );
                new_elements.add( b );

                // combine a previous insert to a range update
                result.add( update_range_operation() )
                      .add( number( index-1 ) )
                      .add( number( index-1 ) )
                      .add( new_elements );
            }
            else if ( prev_op == update_range_operation() )
            {
                array new_elements( last_update.at( 3 ).upcast< array >().copy() );
                new_elements.add( b );

                // or combine a range update with this insert to an range update
                result.add( update_range_operation() )
                      .add( last_update.at( 1 ) )
                      .add( last_update.at( 2 ) )
                      .add( new_elements );
            }
            else
            {
                result.add( insert_at_operation() )
                      .add( number(index) )
                      .add( b );

                prev = ptr;
            }

            assert( result.size() > 3 );

            const vertex newv  = { ptr->length + 1, index + 1, result, prev };

            return newv;
        }

        vertex delete_element( vertex_list_t::const_iterator ptr, int index, const value& prev_op )
        {
            array result;
            vertex_list_t::const_iterator prev = ptr->previous;

            const array last_update = ptr->operation;

            if ( prev_op == update_at_operation() )
            {
                result.add( update_range_operation() )
                      .add( number( index - 1 ) )
                      .add( number( index + 1 ) )
                      .add( array( last_update.at( 2 ) ) );
            }
            else if ( prev_op == delete_at_operation() )
            {
                // combine a previous delete with this delete to a range delete
                result.add( delete_range_operation() )
                      .add( number( index ) )
                      .add( number( index + 2 ) );
            }
            else if ( prev_op == delete_range_operation() )
            {
                // extend a previous range delete to this element
                result.add( delete_range_operation() )
                      .add( last_update.at( 1 ) )
                      .add( increment( last_update.at( 2 ) ) );
            }
            else if ( prev_op == update_range_operation() )
            {
                // we can extend a range update and thus delete this element too
                result.add( update_range_operation() )
                      .add( last_update.at( 1 ) )
                      .add( increment( last_update.at( 2 ) ) )
                      .add( last_update.at( 3 ) );
            }
            else
            {
                result.add( delete_at_operation() )
                      .add( number( index ) );

                prev = ptr;
            }

            assert( !result.empty() );

            const vertex newv  = { ptr->length - 1, index, result, prev };

            return newv;
        }

        bool expand_vertex(vertex_list_t::const_iterator current_state, const array& a, const array& b, vertex_list_t& vertices, cost_list_t& open_list, std::size_t max_size)
        {
            const int inserts_so_far = int(current_state->length) - int(a.length());

            value last_op = current_state->operation.empty() ? null() : current_state->operation.at(0);

            // find position, where a difference between the current state and the target state is
            int index = current_state->index;

            for ( const int max_index = std::min(current_state->length, int(b.length()));
                index != max_index && a.at(index - inserts_so_far) == b.at(index); ++index )
            {
                last_op = null();
            }

            const bool not_at_endof_a = index != int(current_state->length);
            const bool not_at_endof_b = index != int( b.length() );

            if ( !not_at_endof_a && !not_at_endof_b )
                return true;

            // try updating or editing an element from a with an element from b
            if ( not_at_endof_a && not_at_endof_b )
            {
                const json::value at_a = a.at( index - inserts_so_far );
                const json::value at_b = b.at( index );

            	const vertex new_change = change_element(current_state, index, at_a, at_b, last_op != null() );
                add_open(new_change, vertices, open_list, heuristic( a, b, index - inserts_so_far + 1, index + 1 ), max_size );

                std::pair< vertex, bool > new_edited = edit_element( current_state, index, at_a, at_b );

                if ( new_edited.second )
                    add_open(new_edited.first, vertices, open_list, heuristic( a, b, index - inserts_so_far + 1, index + 1 ), max_size );
            }

            // insert an element from b to a at index
            if ( not_at_endof_b )
            {
                const vertex new_insert = insert_element( current_state, index, b.at( index ), last_op );
              	add_open(new_insert, vertices, open_list, heuristic( a, b, index - inserts_so_far, index + 1), max_size );
            }

            // delete an element from a at index
            if ( not_at_endof_a )
            {
                const vertex new_delete = delete_element( current_state, index, last_op );
                add_open(new_delete, vertices, open_list, heuristic( a, b, index - inserts_so_far + 1, index ), max_size );
            }

            return false;
        }

        std::pair<bool, value> delta(const array& a, const array& b, std::size_t max_size)
        {
            if ( a == b )
            {
                return max_size >= 2 
                    ? std::make_pair(true, array())
                    : std::make_pair(false, b);
            }

            const std::size_t first_costs = heuristic(a, b) +1; // plus brackets - comma

            if ( first_costs <= max_size )
            {
                vertex_list_t   vertices;
                cost_list_t     open_list;
                const vertex    start = {a.length(), 0, array(), vertices.end() };
                
                add_open( start, vertices, open_list, first_costs, max_size );

                while ( !open_list.empty() )
                {
                    cost_list_t::const_iterator   next = open_list.begin();
                    vertex_list_t::const_iterator current = *next;
                    open_list.erase(next);

                    if ( expand_vertex(current, a, b, vertices, open_list, max_size) )
                    	return std::make_pair(true, assemble_result(current, vertices));

				}
            }

            return std::make_pair(false, b);
        }

        std::pair<bool, value> delta(const object& a, const object& b, std::size_t max_size)
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

        template <class T>
        struct second_dispatch : default_visitor
        {
            second_dispatch(std::pair<bool, value>& r, const T& a, std::size_t m) 
                : result(r)
                , first_parameter(a)
                , max_size(m)
            {
            }

            std::pair<bool, value>& result;
            const T&                first_parameter;
            const std::size_t       max_size;

            void visit(const T& second)
            {
                result = delta(first_parameter, second, max_size);
            }
        };
    }

    std::pair<bool, value> delta(const value& a, const value& b, std::size_t max_size)
    {
        std::pair<bool, value> result(false, b);

        struct first_dispatch : default_visitor
        {
            first_dispatch(std::pair<bool, value>& r, const value& b, std::size_t m) 
                : result(r) 
                , second_parameter(b)
                , max_size(m)
            {
            }

            std::pair<bool, value>& result;
            const value&            second_parameter;
            const std::size_t       max_size;

            void visit(const object& obj) 
            {
                second_dispatch<object> disp(result, obj, max_size);
                second_parameter.visit(disp);
            }

            void visit(const array& arr)  
            {
                second_dispatch<array> disp(result, arr, max_size);
                second_parameter.visit(disp);
            }
        } dispatch(result, b, max_size);

        a.visit(dispatch);

        return result;
    }

} // namespace json

