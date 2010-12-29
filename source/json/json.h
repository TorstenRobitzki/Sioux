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
#include <stdexcept>

namespace json
{
    class parser;

    class string;
    class number;
    class object;
    class array;
    class true_val;
    class false_val;
    class null;

    class visitor
    {
    public:
        virtual ~visitor() {}
    
        virtual void visit(const string&) = 0;
        virtual void visit(const number&) = 0;
        virtual void visit(const object&) = 0;
        virtual void visit(const array&) = 0;
        virtual void visit(const true_val&) = 0;
        virtual void visit(const false_val&) = 0;
        virtual void visit(const null&) = 0;
    };

    class default_visitor : public visitor
    {
        void visit(const string&) {}
        void visit(const number&) {}
        void visit(const object&) {}
        void visit(const array&) {}
        void visit(const true_val&) {}
        void visit(const false_val&) {}
        void visit(const null&) {}
    };

    class invalid_cast : public std::runtime_error
    {
    public:
        explicit invalid_cast(const std::string& msg);
    };

    class value
    {
    public:
        /**
         * @brief visits the visitor visit function with the underlying type/value
         */
        void visit(visitor&) const;

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

        /**
         * @brief converts this to the requested type.
         * @exception invalid_cast if the underlying type is not the requested type
         */
        template <class TargetType>
        TargetType upcast() const;
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

    /**
     * @relates value
     */
    bool operator==(const value& lhs, const value& rhs);

    /**
     * @relates value
     */
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

        /**
         * @brief returns the integer value of this
         */
        int to_int() const;
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
        object& add(const string& name, const value& val);

        /**
         * @brief returns a list of all keys
         * 
         * The keys will be ordered in descent order
         */
        std::vector<string> keys() const;

        /**
         * @brief removes the element with the given key
         */
        void erase(const string& key);

        /**
         * @brief returns a reference to the element with the given key
         */
        value& at(const string& key);

        /**
         * @brief returns the element with the given key
         */
        const value& at(const string& key) const;
    };

    class array : public value
    {
    public:
        /**
         * @brief an empty array
         */
        array();

        /**
         * @brief constructs an array with one element
         */
        explicit array(const value& first_value);

        /**
         * @brief constructs an array by copying the first references from an other array
         */
        array(const array& original, const std::size_t first_elements);

        /**
         * @brief returns a deep copy of this array.
         *
         * The copied array contains the same references, not a deep copies of 
         * the referenced elements, thus adding an element to the original array
         * is not observable in the copy, but modifing an referenced element will be 
         * observable in the copy.
         */
        array copy() const;

        /**
         * @brief adds a new element to the end of the array
         */
        array& add(const value& val);

        /**
         * @brief returns the number of elements in the array 
         */
        std::size_t length() const;

        /**
         * @brief returns true, if the array is empty (contains no elements)
         */
        bool empty() const;

        /**
         * @brief element with the given index
         */
        const value& at(std::size_t) const;

        /**
         * @brief element with the given index
         */
        value& at(std::size_t);

        /**
         * @brief erases size elements starting with the element at index
         */
        void erase(std::size_t index, std::size_t size);

        /**
         * @brief inserts a new element at index
         */
        void insert(std::size_t index, const value&);

        array& operator+=(const array& rhs);
    private:
        explicit array(impl*);
    };

    array operator+(const array& lhs, const array& rhs);

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
     * @brief constructs a value from a json text
     * @relates value
     */
    value parse(const std::string text);


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

