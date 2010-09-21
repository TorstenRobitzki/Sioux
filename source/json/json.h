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

namespace json
{
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

} // namespace json

#endif // include guard

