// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "tools/substring.h"

namespace server {

    struct header
    {
        header(const tools::substring& name, const tools::substring& value);
        header();

        tools::substring    name_;
        tools::substring    value_;

        tools::substring    name() const;
        tools::substring    value() const;
    };

    /**
     * @brief class that contains a list of header names, that can be used to filter a headers
     */
    class header_filter
    {
    public:
        /**
         * @brief constucts a filter from a comma seperated list of header names
         * 
         * Example header_filter f("Connect, Via, Vary")
         */
        explicit header_filter(const char*);

        /**
         * @brief returns true, if the given header is within the filter
         */
        bool operator()(const header&) const;
    };

} // namespace server

