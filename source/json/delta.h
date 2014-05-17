// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_JSON_DELTA_H
#define SIOUX_SOURCE_JSON_DELTA_H

#include <cstddef>
#include <utility>

namespace json
{
    class value;
    class object;
    class array;

    /**
     * @brief calculates the shortes update sequence that will alter object a to become object b
     * @relates value
     *
     * The function calculates an array, that descibes the updates to perform. The encoding depends on
     * the value types to be compared. For arrays and objectes following update actions are defined:
     * - number(1) : update the element with the name/index of the next value, with the next but one value
     *             example: [1,3, "Nase"] would replace the array element with index 3 with the value "Nase"
     * - number(2) : deletes the element with the name/index of the next value
     *             example: [2,"Nase"] deletes the value with the name "Nase" from an object
     *             example: [2,1] deletes the 2. element from an array or the second character from a string
     * - number(3) : inserts the next but one element at the next index
     *             example: [3,"Nase",[1]] adds a new element named "Nase" with the value [1] to an object
     *
     * In addition, for arrays following range operations are defined
     * - number(4) : deletes the element from the next to the next but one index
     *             example: [4,6,14] deletes the elements with index 6,14 (exclusiv)
     * - number(5) : updates the range from the second to third index (exclusiv) with the 4th value
     *             example: [5,2,3,[1,2]] replaces one element with two elements (1 and 2)
     *
     * For array and object, the edit operation applies the next but one array to the element with the name/index
     * of the next element
     * - number(6) : updates an element
     *             example: [6,2,[3,"Nase",[1]]] executes the update(insert) [3,"Nase",[1]] to the element with the index 2
     *
     * For all othere value types, the function returns b. All array, string indixes are ment with
     * previous updates already commited. If the returned type is not an array, it's ment to replace the
     * element on the left side. So [6,0,{}] would turn [[]] into [{}] or 1 applied to {} would result in 1.
     *
     * The first member or the returned pair indicates whether the function was able to find an update
     * sequence with the given max_size constrain. If it was possible, the first member will be true
     * and the the second member will be an array with a update sequence. If the function fails to
     * calculate an update sequence, the first member will return false and the second member will be
     * set to b.
     */
    std::pair<bool, value> delta(const value& a, const value& b, std::size_t max_size);

    std::pair<bool, value> delta( const object& a, const object& b, std::size_t max_size );

    std::pair<bool, value> delta( const array& a, const array& b, std::size_t max_size );

    /**
     * @brief updates input with the given update_operations
     *
     * If update_operations is an array, update operations are performed on input.
     * In all other cases, update_operations is returned.
     */
    value update(const value& input, const value& update_operations);

    array merge_updates( const array& first, const array& second );

} //namespace json

#endif // include guard

