#ifndef SIOUX_JSON_UDATE_CODES_H
#define SIOUX_JSON_UDATE_CODES_H

namespace json
{
    class number;

    /**
     * @brief Enumeration to name the update operations used by delta() and update()
     * @relates delta
     * @relates update
     */
    enum update_operation_code
    {
        update_at = 1,
        delete_at = 2,
        insert_at = 3,
        delete_range = 4,
        update_range = 5,
        edit_at   =6
    };

    /**
     * @brief returns a reference to a static const json::number(1)
     */
    const number& update_at_operation();

    /**
     * @brief returns a reference to a static const json::number(2)
     */
    const number& delete_at_operation();

    /**
     * @brief returns a reference to a static const json::number(3)
     */
    const number& insert_at_operation();

    /**
     * @brief returns a reference to a static const json::number(4)
     */
    const number& delete_range_operation();

    /**
     * @brief returns a reference to a static const json::number(5)
     */
    const number& update_range_operation();

    /**
     * @brief returns a reference to a static const json::number(6)
     */
    const number& edit_at_operation();
}

#endif
