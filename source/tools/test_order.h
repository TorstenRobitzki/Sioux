// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_TEST_TEST_ORDER_H
#define SIOUX_SOURCE_TEST_TEST_ORDER_H

#include <vector>
#include <functional>
#include <algorithm>

namespace tools 
{
    namespace details {
        template <class T>
        struct compare_pointer : public std::binary_function<const T*, const T*, bool>
        {
            bool operator()(const T* lhs, const T* rhs) const
            {
                return *lhs < *rhs;
            }
        };
    }

    /**
     * @brief checks, that the type that Iter is pointing to implements a strict weak order
     *
     * There must be no duplicates in the range [begin, end). The complexity of the test is
     * O(n^2)
     */
    template <class Iter>
    bool check_weak_order(Iter begin, Iter end)
    {
        typedef typename std::iterator_traits<Iter>::value_type value_type;
        std::vector<value_type*> sorted;
        details::compare_pointer<value_type> compare;

        for ( Iter i = begin; i != end; ++i )
        {
            sorted.push_back(&*i);
        }

        const std::size_t number_of_elements = sorted.size();

        std::sort(sorted.begin(), sorted.end(), compare);

        if ( number_of_elements != sorted.size() )
            return false;

        bool result = true;
        for ( typename std::vector<value_type*>::const_iterator lhs = sorted.begin(); result && lhs != sorted.end(); ++lhs )
        {
            // self test
            result = !compare(*lhs, *lhs);

            for ( typename std::vector<value_type*>::const_iterator rhs = lhs; result && rhs != sorted.end(); ++rhs )
            {
                result = compare(*lhs, *rhs) && !compare(*rhs, *lhs);
            }
        }

        return result;
    }

}

#endif // include guard

