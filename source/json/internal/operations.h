#ifndef SOURCE_JSON_INTERNAL_OPERATIONS_H
#define SOURCE_JSON_INTERNAL_OPERATIONS_H

#include "json/json.h"
#include <boost/shared_ptr.hpp>

namespace json {

    namespace operations {


        class visitor;

        /**
         * @brief base class of a double dispatch hierarchy to alow merge operations
         *
         * This are all internal classes for the delta algorithm on json::arrays
         */
        class update_operation
        {
        public:
            virtual void accept( visitor& ) const = 0;
            virtual void serialize( array& ) const = 0;

            /**
             * @brief merges this and other into one operation that has the very same effect.
             *
             * The default implementation returns 0; The result must have the same effect on an json
             * document as this and the functions argument applied in this order (this, other) would have.
             *
             * Not all possible merge combinations are implemented, only those which are relevant to the delta-function.
             *
             * @return returns 0 if the merge is not possible.
             */
            virtual boost::shared_ptr< update_operation > merge( const update_operation& other ) const;

            virtual ~update_operation() {}
        };

        class update_at : public update_operation
        {
        public:
            update_at( int position, const value& new_value );

            int position() const;
            const value& new_value() const;
        private:
            void accept( visitor& ) const;
            void serialize( array& ) const;
            boost::shared_ptr< update_operation > merge( const update_operation& other ) const;

            int     position_;
            value   new_value_;
        };

        class edit_at : public update_operation
        {
        public:
            edit_at( int position, const value& update_instructions );
        private:
            void accept( visitor& ) const;
            void serialize( array& ) const;

            int     position_;
            value   update_instructions_;
        };

        class delete_at : public update_operation
        {
        public:
            explicit delete_at( int position );

            int position() const;
        private:
            void accept( visitor& ) const;
            void serialize( array& ) const;
            boost::shared_ptr< update_operation > merge( const update_operation& other ) const;

            int     position_;
        };

        class insert_at : public update_operation
        {
        public:
            insert_at( int position, const value& new_value );

            int position() const;
            const value& new_value() const;
        private:
            void accept( visitor& ) const;
            void serialize( array& ) const;
            boost::shared_ptr< update_operation > merge( const update_operation& other ) const;

            int     position_;
            value   new_value_;
        };

        class delete_range : public update_operation
        {
        public:
            delete_range( int from, int to );

            int from() const;
            int to() const;
        private:
            void accept( visitor& ) const;
            void serialize( array& ) const;
            boost::shared_ptr< update_operation > merge( const update_operation& other ) const;

            int from_;
            int to_;
        };

        class update_range : public update_operation
        {
        public:
            update_range( int from, int to, const array& values );

            int from() const;
            int to() const;
            const array& new_values() const;
        private:
            void accept( visitor& ) const;
            void serialize( array& ) const;
            boost::shared_ptr< update_operation > merge( const update_operation& other ) const;

            int     from_;
            int     to_;
            array   new_values_;
        };

        array& operator<<( array&, const update_operation& );

        class visitor
        {
        public:
            void apply( const update_operation& op ) { op.accept( *this ); }
            virtual void visit( const update_at& );
            virtual void visit( const edit_at& );
            virtual void visit( const delete_at& );
            virtual void visit( const insert_at& );
            virtual void visit( const delete_range& );
            virtual void visit( const update_range& );

            virtual ~visitor() {}
        };

    }
}

#endif
