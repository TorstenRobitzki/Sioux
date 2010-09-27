// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_JSON_DELTA_H
#define SIOUX_SOURCE_JSON_DELTA_H

#include <cstddef>

namespace json
{
    class value;

    /**
     * @brief calculates the shortes update sequence that will alter object a to become object b
     * @relates value
     *
     * The function calculates an array, that descibes the updates to perform. The encoding depends on
     * the value types to be compared. Only values of the same type can be alter. For arrays, objectes and
     * strings following update performances are defined:
     * number(1) : update the element with the name/index of the next value, with the next but one value
     *             example: [1,3, "Nase"] would replace 
     * number(2) : deletes the element with the name/index of the next value
     *             example: [2,"Nase"] deletes the value with the name "Nase" from an object
     *             example: [2,1] deletes the 2. element from an array or the second character from a string
     * number(3) : inserts the next but one element at the next index
     *             example: [3,"Nase",[1]] adds a new element named "Nase" with the value [1] to an object
     *
     * In addition, for array and string following range operations are defined
     * number(4) : deletes the element from the next to the next but one index
     *             example: [4,6,14] deletes the elements with index 6,14 (exclusiv)
     * number(5) : updates the range from the second to third index with the 4th value
     *             example: [5,2,3,"foo"] replaces the a part of a string, so that "012345" would become "01foo345"
     *             example: [5,2,3,[1,2]] replaces parts of an array
     *
     * For array and object, the edit operation applies the next but one array to the element with the name/index 
     * of the next element
     * number(6) : updates an element
     *             example: [6,2,[3,"Nase",[1]]] executes the update(insert) [3,"Nase",[1]] to the element with the index 2
     *
     * For all othere value types, the function returns null to indicate failure. All array, string indixes are ment with 
     * previous updates already commited.
     */
    value delta(const value& a, const value& b, std::size_t max_size);

    /**
     * @brief updates input with the given update_operations
     *
     * If update_operations is an array, update operations are performed on input.
     * In all other cases, update_operations is returned.
     */
    value update(const value& input, const value& update_operations);

} //namespace json 

#endif // include guard

