#include "json/delta.h"
#include "json/json.h"
#include "json/internal/operations.h"
#include "json/internal/heuristic.h"
#include "tools/asstring.h"
#include <stdexcept>
#include <set>

namespace json {


    namespace {
        struct vertex;

        typedef std::set< vertex > vertex_list_t;

        struct v_compare : std::binary_function<bool, vertex_list_t::const_iterator, vertex_list_t::const_iterator>
        {
            bool operator()(vertex_list_t::const_iterator lhs, vertex_list_t::const_iterator rhs) const;
        };

        typedef std::multiset< vertex_list_t::const_iterator, v_compare > cost_list_t;

        struct vertex
        {
            // the total length of the modified array, this value is need to find cycles in the search graph.
            int                             length;
            // all elements before this index are equal in the modified array and in the target array
            int                             index;

            // if operation is not null, previous must point to a valid vertex and when vertex points to
            // a valid vertex, the operation must not be null.
            mutable boost::shared_ptr< const operations::update_operation >   operation;
            mutable vertex_list_t::const_iterator   previous;

            mutable std::size_t                     total_costs;
            mutable std::size_t                     costs;

            /**
             * an order by identity
             */
            bool operator<( const vertex& rhs ) const
            {
                return length < rhs.length
                    || length == rhs.length && index < rhs.index;
            }

            void update( const vertex& other ) const
            {
                operation = other.operation;
                previous = other.previous;
                total_costs = other.total_costs;
                costs = other.costs;
            }

            /**
             * contructor for the initial vertex
             */
            vertex( int initial_length, const vertex_list_t& vertexes )
                : length( initial_length )
                , index( 0 )
                , operation()
                , previous( vertexes.end() )
                , total_costs( array().size() )
                , costs( total_costs )
            {}

            vertex( int l, int i, const boost::shared_ptr< const operations::update_operation >& o,
                vertex_list_t::const_iterator p, std::size_t t, std::size_t c )
                : length( l )
                , index( i )
                , operation( o )
                , previous( p )
                , total_costs( t )
                , costs( c )
            {
                assert( o.get() );
            }


        };

        // order by costs
        bool v_compare::operator()(vertex_list_t::const_iterator lhs, vertex_list_t::const_iterator rhs) const
        {
            return lhs->total_costs < rhs->total_costs;
        }

        class array_diff
        {
        public:
            array_diff( const array& a, const array& b, std::size_t max_size )
                : a_( a )
                , b_( b )
                , heuristic_( a, b )
                , max_size_( max_size )
                , vertices_()
                , open_list_()
            {
                const vertex start( a.length(), vertices_ );

                if ( start.costs <= max_size_  )
                {
                    vertices_.insert( start );
                    open_list_.insert( vertices_.begin() );
                }
            }

            std::pair<bool, value> operator()()
            {
                while ( !open_list_.empty() )
                {
                    const vertex_list_t::const_iterator current = remove_from_open_list();

                    if ( expand_vertex( current ) )
                        return std::make_pair( true, assemble_result( current ) );
                }

                return std::make_pair( false, b_ );
            }

        private:
            array assemble_result( vertex_list_t::const_iterator goal ) const
            {
                std::vector< const operations::update_operation* > stack;

                for ( vertex_list_t::const_iterator i = goal ; i->operation.get(); i = i->previous )
                    stack.push_back( i->operation.get() );

                array result;
                for ( std::vector< const operations::update_operation* >::const_reverse_iterator i = stack.rbegin();
                    i != stack.rend(); ++i )
                    ( *i )->serialize( result );

                assert( result.size() == goal->costs );

                return result;
            }

            vertex_list_t::const_iterator remove_from_open_list()
            {
                const vertex_list_t::const_iterator result = *open_list_.begin();
                open_list_.erase( open_list_.begin() );

                return result;
            }

            void add_open( const vertex_list_t::const_iterator current_state, int delta_length, int index, const operations::update_operation* op )
            {
                assert( op );

                boost::shared_ptr< const operations::update_operation > operation( op );
                vertex_list_t::const_iterator   previous = current_state;

                // can we merge this operation with a previous operation?
                if ( current_state->operation.get() )
                {
                    boost::shared_ptr< operations::update_operation > merged_operation =
                        current_state->operation->merge( *op );

                    if ( merged_operation.get() )
                    {
                        operation = merged_operation;
                        previous  = current_state->previous;
                    }
                }

                const int length         = current_state->length + delta_length;
                const int inserts_so_far = length - int( a_.length() );

                const std::size_t costs = previous->costs + operation->size() - 2
                    + ( previous->operation.get() ? 1 : 0 ); // minus brackets + (optional )comma
                const std::size_t estimated_costs = heuristic_( index - inserts_so_far, index );

                const vertex v( length, index, operation, previous, estimated_costs + costs, costs );

                vertex_list_t::iterator insert_pos = vertices_.find( v );

                if ( insert_pos == vertices_.end() || insert_pos->costs > costs )
                {
                    if ( insert_pos != vertices_.end() )
                    {
                        insert_pos->update( v );
                    }
                    else
                    {
                        insert_pos = vertices_.insert( v ).first;
                    }

                    open_list_.insert( insert_pos );
                }
            }

            bool expand_vertex( const vertex_list_t::const_iterator current_state )
            {
                const int inserts_so_far = int( current_state->length ) - int( a_.length() );

                // find a position, where a difference between the current state and the target state is
                int index = current_state->index;

                for ( const int max_index = std::min( current_state->length, int( b_.length() ) );
                    index != max_index && a_.at( index - inserts_so_far ) == b_.at( index ); ++index )
                ;

                const bool not_at_endof_a = index != int( current_state->length );
                const bool not_at_endof_b = index != int( b_.length() );

                if ( !not_at_endof_a && !not_at_endof_b )
                    return true;

                // try updating or editing an element from a with an element from b
                if ( not_at_endof_a && not_at_endof_b )
                {
                    const value& at_a = a_.at( index - inserts_so_far );
                    const value& at_b = b_.at( index );

                    add_open( current_state, 0, index + 1, new operations::update_at( index, at_b ) );

                    const std::pair< bool, value > try_edit = delta( at_a, at_b, max_size_ - current_state->costs );

                    if ( try_edit.first )
                        add_open( current_state, 0, index + 1, new operations::edit_at( index, try_edit.second ) );
                }

                // insert an element from b to a at index
                if ( not_at_endof_b )
                {
                    add_open( current_state, 1, index + 1, new operations::insert_at( index, b_.at( index ) ) );
                }

                // delete an element from a at index
                if ( not_at_endof_a )
                {
                    add_open( current_state, -1, index, new operations::delete_at( index ) );
                }

                return false;
            }

            const array&                a_;
            const array&                b_;
            const details::heuristic    heuristic_;
            const std::size_t           max_size_;
            vertex_list_t               vertices_;
            cost_list_t                 open_list_;
        };

        template < class T >
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

    std::pair< bool, value > delta( const value& a, const value& b, std::size_t max_size )
    {
        std::pair< bool, value > result( false, b );

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
                second_dispatch< object > disp( result, obj, max_size );
                second_parameter.visit( disp );
            }

            void visit(const array& arr)
            {
                second_dispatch< array > disp( result, arr, max_size );
                second_parameter.visit( disp );
            }
        } dispatch(result, b, max_size);

        a.visit(dispatch);

        return result;
    }

    std::pair<bool, value> delta( const array& a, const array& b, std::size_t max_size )
    {
        array_diff diff( a, b, max_size );
        return diff();
    }

} // namespace json

