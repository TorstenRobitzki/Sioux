// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "json/delta.h"
#include "json/json.h"
#include "tools/asstring.h"
#include <stdexcept>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/astar_search.hpp>

namespace json {


    namespace {
        enum update_operation_code
        {
            update_at = 1,
            delete_at = 2,
            insert_at = 3,
            delete_range = 4,
            update_range = 5,
            edit_at   =6
        };

        const number& update_at_operation()
        {
            static const number result(update_at);
            return result;
        }

        const number& delete_at_operation()
        {
            static const number result(delete_at);
            return result;
        }

        const number& insert_at_operation()
        {
            static const number result(insert_at);
            return result;
        }

        const number& delete_range_operation()
        {
            static const number result(delete_range);
            return result;
        }

        const number& update_range_operation()
        {
            static const number result(update_range);
            return result;
        }

        const number& edit_at_operation()
        {
            static const number result(edit_at);
            return result;
        }

        int to_int(const value& val)
        {
            return val.upcast<number>().to_int();
        }

        value update_impl(const array& data, const array& update_operations)
        {
            array result(data.copy());

            for ( std::size_t index = 0; index < update_operations.length(); )
            {
                const int command = to_int(update_operations.at(index++));

                switch ( command )
                {
                case update_at:
                    {
                        const int update_idx = to_int(update_operations.at(index++));
                        result.at(update_idx) = update_operations.at(index++);
                    }
                    break;
                case delete_at:
                    {
                        const int delete_idx = to_int(update_operations.at(index++));
                        result.erase(delete_idx, 1u);
                    }
                    break;
                case insert_at:
                    {
                        const int insert_idx = to_int(update_operations.at(index++));
                        result.insert(insert_idx, update_operations.at(index++));
                    }
                    break;
                case delete_range:
                    {
                        const int start_idx = to_int(update_operations.at(index++));
                        const int end_idx = to_int(update_operations.at(index++));
                        result.erase(start_idx, end_idx-start_idx);
                    }
                    break;
                case update_range:
                    {
                        int start_idx = to_int(update_operations.at(index++));
                        const int end_idx = to_int(update_operations.at(index++));
                        result.erase(start_idx, end_idx-start_idx);

                        const array& fill = update_operations.at(index++).upcast<array>();
                        for ( int i = 0, end = fill.length(); i != end; ++i, ++start_idx )
                            result.insert(start_idx, fill.at(i));
                    }
                    break;
                case edit_at:
                    {
                        const int update_idx = to_int(update_operations.at(index++));
                        const value& update_operation  = update_operations.at(index++);

                        result.at(update_idx) = update(result.at(update_idx), update_operation);
                    }
                    break;
                default:
                    throw std::runtime_error("invalid update operation: " + tools::as_string(command));
                }
            }

            return result;
        }

        value update_impl(const object& data, const array& update_operations)
        {
            object result(data);

            for ( std::size_t index = 0; index < update_operations.length(); )
            {
                const int command = to_int(update_operations.at(index++));

                switch ( command )
                {
                case update_at:
                    {
                        const string update_idx = update_operations.at(index++).upcast<string>();
                        result.at(update_idx) = update_operations.at(index++);
                    }
                    break;
                case delete_at:
                    {
                        const string delete_idx = update_operations.at(index++).upcast<string>();
                        result.erase(delete_idx);
                    }
                    break;
                case insert_at:
                    {
                        const string insert_idx = update_operations.at(index++).upcast<string>();
                        result.add(insert_idx, update_operations.at(index++));
                    }
                    break;
                case edit_at:
                    {
                        const string update_idx = update_operations.at(index++).upcast<string>();
                        const value& update_operation  = update_operations.at(index++);

                        result.at(update_idx) = update(result.at(update_idx), update_operation);
                    }
                    break;
                default:
                    throw std::runtime_error("invalid update operation: " + tools::as_string(command));
                }
            }

            return result;
        }

        value update_impl(const value& a, const array& update_operations)
        {
            struct update_by_type : default_visitor
            {
                update_by_type(const array& update_instructions)
                    : instructions_(update_instructions)
                    , result_(update_instructions)
                {
                }

                void visit(const object& val)
                {
                    result_ = update_impl(val, instructions_);
                }

                void visit(const array& val)
                {
                    result_ = update_impl(val, instructions_);
                }

                const array&    instructions_;
                value           result_;
            } visi(update_operations);

            a.visit(visi);
            return visi.result_;
        }
    }

    value update(const value& a, const value& update_operations)
    {
        struct is_array_visitor : default_visitor
        {
            is_array_visitor(const value& a, const value& update_operations)
                : arg_(a)
                , result_(update_operations)
            {
            }

            void visit(const array& update_operations) 
            {
                result_ = update_impl(arg_, update_operations);
            }

            const value&    arg_;
            value           result_;

        } v(a, update_operations);

        update_operations.visit(v);

        return v.result_;
    }

    namespace {
        struct vertex;

        typedef std::set<vertex> vertex_list_t;

        struct v_compare : std::binary_function<bool, vertex_list_t::const_iterator, vertex_list_t::const_iterator>
        {
            bool operator()(vertex_list_t::const_iterator lhs, vertex_list_t::const_iterator rhs) const;
        };

        typedef std::multiset<vertex_list_t::const_iterator, v_compare> cost_list_t;

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

            bool operator<(const vertex& rhs) const
            {
                return length < rhs.length
                    || length == rhs.length && index < rhs.index
                    || length == rhs.length && index && rhs.index && total_costs < rhs.total_costs;
            }
        };

        bool v_compare::operator()(vertex_list_t::const_iterator lhs, vertex_list_t::const_iterator rhs) const
        {
            return lhs->total_costs < rhs->total_costs;
        }

        void add_open(const vertex& v, vertex_list_t& vlist, cost_list_t& clist)
        {
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

        /// @todo implement hystereses
        std::size_t hysterese(const array& /*a*/, const array& /*b*/, int /*a_index*/, int /*b_index*/)
        {
            return 0;
        }

        std::size_t hysterese(const array& a, const array& b)
        {
            return hysterese(a, b, 0, 0);
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

        vertex change_element(vertex_list_t::const_iterator ptr, int index, const value& a, const value& b, const value& prev_op, std::size_t hysteresis) 
        {
            assert(a != b);

            array                           result;
            vertex_list_t::const_iterator   prev        = ptr->previous;
            const array                     last_update = ptr->operation;

            if ( prev_op == update_at_operation() )
            {
                // combine a previous update with this update to a range update
                array new_elements;
                new_elements.add(last_update.at(2)).add(b);

                result.add(update_range_operation())
                      .add(last_update.at(1))
                      .add(number(index+1))
                      .add(new_elements);
            }
            else if ( prev_op == insert_at_operation() )
            {
                // combine a previous insert with this update to a range update
                array new_elements(last_update.at(2));
                new_elements.add(b);

                assert(index > 0);

                result.add(update_range_operation())
                      .add(number(index-1))
                      .add(number(index+1))
                      .add(new_elements);
            }
            else if ( prev_op == delete_at_operation() )
            {
            	// combine a previous delete with this update to a range update
                result.add(update_range_operation())
                      .add(number(index))
                      .add(number(index+2))
                      .add(array(b));
            }
            else if ( prev_op == delete_range_operation() )
            {
                // combine a previous range delete with this update to a range update
                result.add(update_range_operation())
                      .add(last_update.at(1))
                      .add(increment(last_update.at(2)))
                      .add(array(b));
            }
            else if ( prev_op == update_range_operation() )
            {
                array new_elements(last_update.at(3).upcast<array>().copy());
                new_elements.add(b);

                // combine a previous update with this update to a range update
                result.add(update_range_operation())
                      .add(last_update.at(1))
                      .add(increment(last_update.at(2)))
                      .add(new_elements);
            }
            else
            {
                result.add(update_at_operation()).add(number(index)).add(b);
                prev = ptr;
            }

            assert(!result.empty());
            size_t costs = prev->costs + result.size() -2 +1; // minus bracket plus a comma

            // lets try, if we can do better with an edit
            std::pair<bool, value> edit_operation = delta(a, b, costs-5 /* the minimum costs of the edit framing (,6,0,) */);

            if ( edit_operation.first )
            {
                array edit_result;
                edit_result.add(edit_at_operation())
                           .add(number(index))
                           .add(edit_operation.second);

                const std::size_t edit_costs = prev->costs + edit_result.size() -2 +1;

                if ( edit_costs < costs )
                {
                    result = edit_result;
                    prev   = ptr;
                    costs  = edit_costs;
                }
            }

            const vertex newv  = {ptr->length, index+1, result, prev, costs + hysteresis, costs};

            return newv;
        }

        vertex insert_element(vertex_list_t::const_iterator ptr, int index, const value& b, const value& prev_op, std::size_t hysterese)
        {
            array result;
            vertex_list_t::const_iterator prev = ptr->previous;

            const array last_update = ptr->operation;

            // a combination of a delete/range delete with an insert to an range update is 
            // not implemented, because an update should be considered too

            if ( prev_op == update_at_operation() )
            {
            	array new_elements(last_update.at(2));
                new_elements.add(b);

                result.add(update_range_operation())
                      .add(last_update.at(1))
                      .add(number(index))
                      .add(new_elements);
            }
            else if ( prev_op == insert_at_operation() )
            {
                array new_elements(last_update.at(2));
                new_elements.add(b);

                // combine a previous insert to a range update
                result.add(update_range_operation())
                      .add(number(index-1))
                      .add(number(index-1))
                      .add(new_elements);
            }
            else if ( prev_op == update_range_operation() )
            {
                array new_elements(last_update.at(3).upcast<array>().copy());
                new_elements.add(b);

                // or combine a range update with this insert to an range update
                result.add(update_range_operation())
                      .add(last_update.at(1))
                      .add(last_update.at(2))
                      .add(new_elements);
            }
            else
            {
                result.add(insert_at_operation())
                      .add(number(index))
                      .add(b);

                prev = ptr;
            }

            assert(result.size() > 3);

            const size_t costs = prev->costs + result.size() -2 +1; // minus bracket plus a comma
            const vertex newv  = {ptr->length+1, index+1, result, prev, costs + hysterese, costs};

            return newv;
        }

        vertex delete_element(vertex_list_t::const_iterator ptr, int index, const value& prev_op, std::size_t hysterese)
        {
            array result;
            vertex_list_t::const_iterator prev = ptr->previous;

            const array last_update = ptr->operation;

            if ( prev_op == update_at_operation() )
            {
                result.add(update_range_operation())
                      .add(number(index-1))
                      .add(number(index+1))
                      .add(array(last_update.at(2)));
            }
            else if ( prev_op == delete_at_operation() )
            {
                // combine a previous delete with this delete to a range delete
                result.add(delete_range_operation())
                      .add(number(index))
                      .add(number(index+2));
            }
            else if ( prev_op == delete_range_operation() )
            {
                // extend a previous range delete to this element
                result.add(delete_range_operation())
                      .add(last_update.at(1))
                      .add(increment(last_update.at(2)));
            }
            else if ( prev_op == update_range_operation() )
            {
                // we can extend a range update and thus delete this element too
                result.add(update_range_operation())
                      .add(last_update.at(1))
                      .add(increment(last_update.at(2)))
                      .add(last_update.at(3));
            }
            else
            {
                result.add(delete_at_operation())
                      .add(number(index));

                prev = ptr;
            }

            assert(!result.empty());

            const size_t costs = prev->costs + result.size() -2 +1; // minus bracket plus a comma
            const vertex newv  = {ptr->length-1, index, result, prev,costs + hysterese, costs};

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

            if ( index == int(b.length()) && index == int(current_state->length) )
                return true;

            // try updating an element from a with an element from b
            if ( index != int(b.length()) && index != int(current_state->length) )
            {
            	const vertex new_change = change_element(current_state, index, a.at(index-inserts_so_far), b.at(index),
                    last_op, hysterese(a,b,index-inserts_so_far+1,index+1));

                if ( new_change.costs <= max_size )
                    add_open(new_change, vertices, open_list);
            }

            // insert an element from b to a at index
            if ( index != int(b.length()))
            {
                const vertex new_insert = insert_element(current_state, index, b.at(index), 
                    last_op, hysterese(a,b,index-inserts_so_far,index+1));

                if ( new_insert.costs <= max_size )
                	add_open(new_insert, vertices, open_list);
            }

            // delete an element from a at index
            if ( index != int(current_state->length) )
            {
                const vertex new_delete = delete_element(current_state, index, 
                    last_op, hysterese(a,b,index-inserts_so_far+1,index));

                if ( new_delete.costs <= max_size )
                    add_open(new_delete, vertices, open_list);
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

            const std::size_t first_costs = hysterese(a, b) +1; // plus brackets - comma

            if ( first_costs <= max_size )
            {
                vertex_list_t   vertices;
                cost_list_t     open_list;
                const vertex    start = {a.length(), 0, array(), vertices.end(), first_costs, 1};
                
                add_open(start, vertices, open_list);

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
                    result.add(delete_at_operation())
                          .add(*pa);

                    ++pa;
                }
                else if ( pa == akeys.end() || pb != bkeys.end() && *pb < *pa )
                {
                    result.add(insert_at_operation())
                          .add(*pb)
                          .add(b.at(*pb));

                    ++pb;
                }
                else
                {
                    assert(*pa == *pb);
                    const value b_element = b.at(*pb);

                    std::pair<bool, value> edit_op = delta(a.at(*pa), b_element, max_size - result.size());

                    // use edit, if possible and shorter
                    if ( edit_op.first && edit_op.second.size() < b_element.size() )
                    {
                        result.add(edit_at_operation())
                              .add(*pa)
                              .add(edit_op.second);

                    }
                    else
                    {
                        result.add(update_at_operation())
                              .add(*pa)
                              .add(b_element);
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

