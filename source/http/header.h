// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SRC_HTTP_HEADER_H
#define SIOUX_SRC_HTTP_HEADER_H

#include "tools/substring.h"
#include <vector>

namespace http {

    /**
     * @brief structure that points to the relevant values of a http header in a text buffer
     */
    struct header
    {
        /**
         * @brief constructs a header
         *
         * @param all the whole header including leading spaces and trailing \\r\n
         * @param name the name, without leading spaces and without trailing spaces or colon
         * @param value the value without leading spaces or colon and withour trailing \\r\n
         */
        header(const tools::substring& all, const tools::substring& name, const tools::substring& value);

        header();

        /**
         * @brief searches for the colon and fills all_, with (begin,end] and name_ and value_
         * according. Leading and trailing \\r\n, \\t and spaces are removed by altering the begin()
         * and end() values of name_ and values_
         *
         * The function returns false, if no colon is found, if name is empty or if begin is equal to ' ' or '\\t'
         */
        bool parse(const char* begin, const char* end);

        /**
         * @brief expands the current value_ to the new_end and removes trailing spaces and expands all_ to the new end.
         */
        void add_value_line(const char* new_end);

        tools::substring    all_;
        tools::substring    name_;
        tools::substring    value_;

        tools::substring    all() const;
        tools::substring    name() const;
        tools::substring    value() const;

        tools::substring::iterator  begin() const;
        tools::substring::iterator  end() const;
    };

} // namespace http

#endif // include guard
