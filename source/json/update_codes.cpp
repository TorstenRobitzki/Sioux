#include "json/update_codes.h"
#include "json/json.h"

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

}
