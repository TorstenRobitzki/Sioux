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

/** @namespace json */
namespace json
{
    class parser;

    class value;
    class string;
    class number;
    class object;
    class array;
    class true_val;
    class false_val;
    class null;

    /**
     * @brief interface to examine a value
     */
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

    /**
     * @brief thrown in case, a value is casted to a static type that is not satisfied by the runtime type
     */
    class invalid_cast : public std::runtime_error
    {
    public:
        explicit invalid_cast(const std::string& msg);
    };

    /**
     * @brief abstract json value. Serves as base class and for a place holder for every other concrete json value.
     */
    class value
    {
    public:
        /**
         * @brief visits the visitor visit function with the underlying type/value
         */
        void visit(visitor&) const;

        /**
         * @brief a defined, but unspecified, strict, weak order
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

        /**
         * @brief swaps the guts of this and other
         * @exception none
         */
        void swap( value& other );
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

    /**
     * @brief representation of a json string object
     */
    class string : public value
    {
    public:
        /**
         * @brief an empty string
         */
        string();

        explicit string(const char*);

        /**
         * @brief returns true, if the number of stored characters is zero
         */
        bool empty() const;

        /**
         * @brief returns a string containing the same character sequence as the json string
         *
         * But instead to to_json() is the text not json encoded.
         */
        std::string to_std_string() const;
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

    /**
     * @brief a representation of a json object ( name / value hash set )
     */
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
         * @exception std::out_of_range
         */
        value& at(const string& key);

        /**
         * @brief returns the element with the given key
         * @exception std::out_of_range
         */
        const value& at(const string& key) const;

        /**
         * @brief looks up the given key and returns a pointer to it
         *
         * The function will return 0, if no value with the given key is given.
         */
        value* find( const string& key );

        /**
         * @copydoc find( const string& key )
         */
        const value* find( const string& key ) const;

        /**
         * @brief returns a deep copy of this object.
         *
         * The copied object contains the same references, not a deep copies of
         * the referenced elements, thus adding an element to the original object
         * is not observable in the copy, but modifing an referenced element will be
         * observable in the copy.
         */
        object copy() const;

        /**
         * @brief returns true, if the object contains not key value pair
         *
         * equal to *this == parse( "{}" )
         */
        bool empty() const;
    private:
        explicit object(impl*);
    };

    /**
     * @brief array of references to values
     */
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
         * @brief constructs an array by copying the first references starting at start_idx from an other array
         */
        array(const array& other, const std::size_t number_to_copy, const std::size_t start_idx);

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

        /**
         * @brief invokes e.visit(v) for ever element e in the array
         */
        void for_each( visitor& v ) const;
    private:
        explicit array(impl*);
    };

    /**
     * @brief forms a new array, beginning with the elements of lhs followed by the elements from rhs
     *
     * The resulting array keeps references to the very same element of lhs and rhs.
     * @relates array
     */
    array operator+(const array& lhs, const array& rhs);

    /**
     * @brief class representing the java script value 'true'
     *
     * The only useful property of true_val is to compare true with every
     * other instance of true_val and to be not equal to every other implementation
     * of value.
     */
    class true_val : public value
    {
    public:
        true_val();
    };

    /**
     * @brief class representing the java script value 'false'
     *
     * The only useful property of false_val is to compare true with every
     * other instance of false_val and to be not equal to every other implementation
     * of value.
     */
    class false_val :  public value
    {
    public:
        false_val();
    };

    /**
     * @brief class representing the java script value 'null'
     *
     * The only useful property of null is to compare true with every
     * other instance of null and to be not equal to every other implementation
     * of value.
     */
    class null : public value
    {
    public:
        null();
    };

    /** 
     * @brief an error is occurred, while parsing a json text
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
         *
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

    /**
     * @brief first substitutes all occurens of the ' (single quote) with a " (double quote) and than passes the
     *        result to parse()
     *
     * So for example the json object { "a":"b"; "c":1 } could be constructed out of the string literal
     * "{'a':'b'; 'c':1}", instead of the harder to read one with escaped double quotes "\"a\":\"b\"; \"b\‚\":1".
     */
    value parse_single_quoted( const std::string single_quoted_string );

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
