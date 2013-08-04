#ifndef SOURCE_JSON_MERGE_OPERATIONS_H
#define SOURCE_JSON_MERGE_OPERATIONS_H

namespace json {
    class array;
    class value;

    namespace details {


        /**
         * @brief tries to merge an update operation, with the last operation
         */
        bool merge_change_element( const array& last_update, int index, const value& b, array& result );
    }
}

#endif

