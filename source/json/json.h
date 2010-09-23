// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_JSON_JSON_H
#define SIOUX_SOURCE_JSON_JSON_H

#include <boost/shared_ptr.hpp>
#include <boost/asio/buffer.hpp>
#include <cstddef>
#include <vector>
#include <string>
#include <iosfwd>
#include <stack>

namespace json
{
    class parser;

    class value
    {
    public:
        /**
         * @brief a defined, but unspecified, striked, weak order
         */
        bool operator<(const value& rhs) const;

        ~value();

        class impl;

        /**
         * @brief this in bytes of the serialized form in bytes
         */
        std::size_t size() const;

        /**
         * @brief serialized form of the data
         * @attention do not use the buffer, after the serialized data is destroyed or altered.
         */
        void to_json(std::vector<boost::asio::const_buffer>& const_buffer_sequence) const;

        /**
         * @brief converts the content to json
         * @attention this is for test purposes only
         */
        std::string to_json() const;
    protected:
        explicit value(impl*);
        explicit value(const boost::shared_ptr<impl>& impl);

        template <class Type>
        Type& get_impl();

        template <class Type>
        const Type& get_impl() const;
    private:
        boost::shared_ptr<impl> pimpl_;

        friend class parser;
    };

    bool operator==(const value& lhs, const value& rhs);
    bool operator!=(const value& lhs, const value& rhs);

    /**
     * @brief prints the content of the json value onto the given stream for debug purpose
     * @relates value
     */
    std::ostream& operator<<(std::ostream& out, const value&);

    class string : public value
    {
    public:
        /**
         * @brief an empty string
         */
        string();

        explicit string(const char*);
    };

    class number : public value
    {
    public:
        /**
         * @brief constructs a number from an integer value
         */
        explicit number(int value);

        /**
         * @brief constructs a number from a double value
         */
        explicit number(double value);
    };

    class object : public value
    {
    public:
        /**
         * @brief an empty object
         */
        object();

        /**
         * @brief adds a new property to the object
         */
        void add(const string& name, const value& val);
    };

    class array : public value
    {
    public:
        /**
         * @brief an empty array
         */
        array();

        /**
         * @brief adds a new element to the end of the array
         */
        void add(const value& val);
    };

    class true_val : public value
    {
    public:
        true_val();
    };

    class false_val :  public value
    {
    public:
        false_val();
    };

    class null : public value
    {
    public:
        null();
    };

    /** 
     * @brief an error is occured, while parsing a json text
     */
    class parse_error : public std::runtime_error
    {
    public:
        explicit parse_error(const std::string&);
    };

    /**
     * @brief a state full json parser
     */
    class parser
    {
    public:
        parser();

        bool parse(const char* begin, const char* end);

        template <class Iter>
        bool parse(Iter begin, Iter end)
        {
            const std::vector<char> buffer(begin, end);
            return parse(&buffer[0], &buffer[0] + buffer.size());
        }

        /**
         * @brief indicates that no more data will follow
         *
         * For a JSON number, there is no way to detect that the text is fully parsed,
         * so if it's valid, that a number is to be parsed, flush() have to be called, 
         * when no more data is expected.
         * @exception parse_error if the parsed text up to now isn't a valid json text
         */
        void flush();

        /**
         * @brief returns the parsed value,
         * @pre parse() returned true or flush() was called without causing an error
         */
        value result() const;
    private:
        int parse_idle(char c);
        const char* parse_number(const char* begin, const char* end);
        const char* parse_array(const char* begin, const char* end);
        const char* parse_object(const char* begin, const char* end);
        const char* parse_string(const char* begin, const char* end);
        const char* parse_literal(const char* begin, const char* end);

        void value_parsed(const value&);

        std::vector<char>   buffer_;
        std::stack<value>   result_;
        std::stack<int>     state_;
    };

    /**
     * @brief constructs a value from a json text
     * @relates value
     */
    template <class Iter>
    value parse(Iter begin, Iter end);

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
     *             example: [4,6,14] deletes the elements with index 6,14 (inclusiv)
     * number(5) : updates the range from the second to third index with the 4th value
     *             example: [5,2,3,"foo"] replaces the a part of a string, so that "012345" would become "01foo345"
     *             example: [5,2,3,[1,2]] replaces parts of an array
     *
     * For array and object, the edit operation applies the next but one array to the element with the name/index 
     * of the next element
     * number(6) : updates an element
     *             example: [6,2,[3,"Nase",[1]]] executes the update(insert) [3,"Nase",[1]] to the element with the index 2
     *
     * For all othere value types, the function returns null to indicate failure.
     */
    value difference(const value& a, const value& b, std::size_t max_size);

    value update(const value& a, const value& update_operations);


    template <class Iter>
    value parse(Iter begin, Iter end)
    {
        parser p;

        if ( !p.parse(begin, end) )
            p.flush();

        return p.result();
    }

} // namespace json

#endif // include guard

