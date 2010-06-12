// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "tools/substring.h"

namespace http {

    /**
     * @brief class that contains a list of header names, that can be used to filter a headers
     *
     * Comparison is done with ASCII in mind and by ignoring cases
     */
    class filter
    {
    public:
        /**
         * @brief constucts a filter from a comma seperated list of header names
         * 
         * Example header_filter f("Connect, Via, Vary,\r\nFooBar")
         */
        explicit filter(const char*);

        /**
         * @brief constructs a filter from a comma seperated list of header names
         *
         * @attention the filter creates references into the passed string and thus the string have to
         * stay valid as long, as the filter is valid.
         */
        explicit filter(const tools::substring&);

        /**
         * @brief filter that will return false for every call to operator()
         */
        filter();

        filter(const filter&);

        filter& operator=(const filter&);

        ~filter();

        /**
         * @brief returns true, if the given name is within the filter
         */
        bool operator()(const tools::substring&) const;

        /**
         * @brief adds the elements of the right hand filter to this filter
         */
        filter& operator+=(const filter& rhs);

    private:
        void build_index();
        void add_element(const char* begin, const char* end);

        std::vector<char>               values_;
        std::vector<tools::substring>   index_;
    };

    /**
     * @brief returns a filter that contains all header names from the left and from the right hand argument
     * @relates filter
     */
    filter operator+(const filter& lhs, const filter& rhs);

} // namespace http