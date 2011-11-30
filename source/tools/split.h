// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SOURCE_TOOLS_SPLIT_H
#define SOURCE_TOOLS_SPLIT_H


namespace tools {

    /**
     * @brief splits a given string in two parts: first_part and second_part, where first_part + n-times seperator + 
     * second_part yields input; with n >0 and length of first_part > 0 and length of second_part > 0. If this 
     * conditions can not be met, first_part and second_part will not be altered and the function returns false.
     */
    template <class Iterator, class String>
    bool split(Iterator begin, Iterator end, typename String::value_type seperator, String& first_part, String& second_part)
    {
        Iterator first = begin;
        for ( ; first != end && *first != seperator; ++first )
            ;

        if ( first == begin || first == end )
            return false;

        Iterator second = first +1;
        for ( ; second != end && *second == seperator; ++second )
            ;

        if ( second == end )
            return false;

        first_part = String(begin, first);
        second_part = String(second, end);

        return true;
    }

    /**
     * @brief splits a given string in two parts: first_part and second_part, where first_part + seperator +
     * second_part yields input; with length of first_part >= 0 and length of second_part >= 0. if the seperator
     * can not be found, the function will return false, and will leave first_part and second_part unaltered.
     */
    template <class Iterator, class String>
    bool split_to_empty(Iterator begin, Iterator end, typename String::value_type seperator, String& first_part,
    	String& second_part)
    {
        Iterator first = begin;
        for ( ; first != end && *first != seperator; ++first )
            ;

        if ( first == end )
            return false;

        Iterator second = first;
        ++second;

        first_part = String(begin, first);
        second_part = String(second, end);

        return true;
    }

    template <class String>
    bool split(const String& input, typename String::value_type seperator, String& first_part, String& second_part)
    {
        return split(input.begin(), input.end(), seperator, first_part, second_part);
    }

    template <class String>
    bool split_to_empty(const String& input, typename String::value_type seperator, String& first_part, String& second_part)
    {
        return split_to_empty(input.begin(), input.end(), seperator, first_part, second_part);
    }

} // namespace tools

#endif

