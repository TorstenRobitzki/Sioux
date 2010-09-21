// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "json/json.h"
#include "tools/dynamic_type.h"
#include "tools/asstring.h"
#include <algorithm>
#include <iterator>

namespace json
{
    namespace {
        class string_impl;
        class number_impl;
        class object_impl;
        class array_impl;
        class true_impl;
        class false_impl;
        class null_impl;

        class visitor;
    }
    
    class value::impl
    {
    public:
        enum type_code {
            string_code,
            number_code,
            object_code,
            array_code,
            true_code,
            false_code,
            null_code
        };

        virtual ~impl() {}

        virtual void visit(const visitor&) const = 0;
        virtual std::size_t size() const = 0;
        virtual void to_json(std::vector<boost::asio::const_buffer>& const_buffer_sequence) const = 0;
        virtual type_code code() const = 0;
    };

    namespace {
        class visitor
        {
        public:
            virtual void visit(const string_impl&) const = 0;
            virtual void visit(const number_impl&) const = 0;
            virtual void visit(const object_impl&) const = 0;
            virtual void visit(const array_impl&) const = 0;
            virtual void visit(const true_impl&) const = 0;
            virtual void visit(const false_impl&) const = 0;
            virtual void visit(const null_impl&) const = 0;

            virtual ~visitor() {}
        };


        class string_impl : public value::impl
        {
        public:
            string_impl(const char* s) : data_()
            {
                data_.push_back('\"');
                
                for ( ; *s; ++s )
                {
                    switch ( *s )
                    {
                    case '\"' : 
                        data_.push_back('\\');
                        data_.push_back('\"');
                        break;
                    case '\\' : 
                        data_.push_back('\\');
                        data_.push_back('\\');
                        break;
                    case '/' : 
                        data_.push_back('\\');
                        data_.push_back('/');
                        break;
                    case '\b' : 
                        data_.push_back('\\');
                        data_.push_back('b');
                        break;
                    case '\f' : 
                        data_.push_back('\\');
                        data_.push_back('f');
                        break;
                    case '\n' : 
                        data_.push_back('\\');
                        data_.push_back('n');
                        break;
                    case '\r' : 
                        data_.push_back('\\');
                        data_.push_back('r');
                        break;
                    case '\t' : 
                        data_.push_back('\\');
                        data_.push_back('t');
                        break;
                    default:
                        data_.push_back(*s);
                    }
                }

                data_.push_back('\"');
            }

            bool operator<(const string_impl& rhs) const
            {
                if ( data_.size() != rhs.data_.size() )
                    return data_.size() < rhs.data_.size();

                const std::pair<std::vector<char>::const_iterator, std::vector<char>::const_iterator> found =
                    std::mismatch(data_.begin(), data_.end(), rhs.data_.begin());

                return found.first != data_.end() && *found.first < *found.second;
            }

        private:
            void visit(const visitor& v) const 
            {
                v.visit(*this);
            }

            std::size_t size() const
            {
                return data_.size();
            }

            void to_json(std::vector<boost::asio::const_buffer>& const_buffer_sequence) const
            {
                const_buffer_sequence.push_back(boost::asio::buffer(data_));
            }

            type_code code() const{
                return string_code;
            }

            std::vector<char>   data_;
        };

        ///////////////////////////////////////
        // number_impl
        class number_impl : public value::impl
        {
        public:
            explicit number_impl(int val)
                : data_()   
            {
                if ( val == 0 )
                {
                    data_.push_back('0');
                    return;
                }

                bool negativ = false; 

                if ( val < 0 )
                {
                    negativ = true;
                    val = -val;
                }

                for ( ; val != 0; val = val / 10)
                {
                    data_.insert(data_.begin(), (val % 10) + '0');
                }

                if ( negativ )
                    data_.insert(data_.begin(), '-');
            }

            explicit number_impl(double val)
                : data_()
            {
                const std::string s = tools::as_string(val);
                data_.insert(data_.begin(), s.begin(), s.end());
            }

        private:
            void visit(const visitor& v) const
            {
                v.visit(*this);
            }

            std::size_t size() const
            {
                return data_.size();
            }

            void to_json(std::vector<boost::asio::const_buffer>& const_buffer_sequence) const
            {
                const_buffer_sequence.push_back(boost::asio::buffer(data_));
            }

            type_code code() const 
            {
                return number_code;
            }

            std::vector<char>   data_;
        };

        ////////////////////
        // class object_impl
        class object_impl : public value::impl
        {
        public:
            void add(const string& name, const value& val)
            {
                members_.push_back(std::make_pair(name, val));
            }

        private:
            void visit(const visitor& v) const
            {
                v.visit(*this);
            }

            std::size_t size() const
            {
                std::size_t result = 2 + members_.size();

                if ( members_.size() > 0 )
                    result += members_.size() -1;

                for ( list_t::const_iterator i = members_.begin(); i != members_.end(); ++i )
                {
                    result += i->first.size();
                    result += i->second.size();
                }

                return result;
            }

            void to_json(std::vector<boost::asio::const_buffer>& const_buffer_sequence) const
            {
                static const char comma[] = {','};
                static const char open[]  = {'{'};
                static const char close[] = {'}'};
                static const char colon[] = {':'};

                const_buffer_sequence.push_back(boost::asio::buffer(open));

                for ( list_t::const_iterator i = members_.begin(); i != members_.end(); ++i )
                {
                    i->first.to_json(const_buffer_sequence);
                    const_buffer_sequence.push_back(boost::asio::buffer(colon));
                    i->second.to_json(const_buffer_sequence);

                    if ( i+1 != members_.end() )
                        const_buffer_sequence.push_back(boost::asio::buffer(comma));
                }

                const_buffer_sequence.push_back(boost::asio::buffer(close));
            }

            type_code code() const
            {
                return object_code;
            }

            typedef std::vector<std::pair<string, value> > list_t;
            list_t members_;
        };

        ////////////////////
        // class array_impl
        class array_impl : public value::impl
        {
        public:
            void add(const value& v)
            {
                members_.push_back(v);
            }
        private:
            void visit(const visitor& v) const
            {
                v.visit(*this);
            }

            std::size_t size() const
            {
                std::size_t result = 2;

                if ( members_.size() > 1 )
                    result += members_.size() -1;

                for ( list_t::const_iterator i = members_.begin(); i != members_.end(); ++i)
                {
                    result += i->size();
                }

                return result;
            }

            void to_json(std::vector<boost::asio::const_buffer>& const_buffer_sequence) const
            {
                static const char comma[] = {','};
                static const char open[]  = {'['};
                static const char close[] = {']'};

                const_buffer_sequence.push_back(boost::asio::buffer(open));

                for ( list_t::const_iterator i = members_.begin(); i != members_.end(); ++i )
                {
                    i->to_json(const_buffer_sequence);

                    if ( i+1 != members_.end() )
                        const_buffer_sequence.push_back(boost::asio::buffer(comma));
                }

                const_buffer_sequence.push_back(boost::asio::buffer(close));
            }

            type_code code() const
            {
                return array_code;
            }

            typedef std::vector<value> list_t;
            list_t members_;
        };

        ////////////////////
        // class false_impl
        class false_impl : public value::impl
        {
        private:
            void visit(const visitor& v) const
            {
                v.visit(*this);
            }

            std::size_t size() const
            {
                return 5u;
            }

            void to_json(std::vector<boost::asio::const_buffer>& const_buffer_sequence) const
            {
                static const char text[] = {'f', 'a', 'l', 's', 'e'};

                const_buffer_sequence.push_back(boost::asio::buffer(text));
            }

            type_code code() const
            {
                return false_code;
            }

        };

        ///////////////////
        // class true_impl
        class true_impl : public value::impl
        {
        private:
            void visit(const visitor& v) const
            {
                v.visit(*this);
            }

            std::size_t size() const
            {
                return 4u;
            }

            void to_json(std::vector<boost::asio::const_buffer>& const_buffer_sequence) const
            {
                static const char text[] = {'t', 'r', 'u', 'e'};

                const_buffer_sequence.push_back(boost::asio::buffer(text));
            }

            type_code code() const
            {
                return true_code;
            }

        };

        ///////////////////
        // class null_impl
        class null_impl : public value::impl
        {
        private:
            void visit(const visitor& v) const
            {
                v.visit(*this);
            }

            std::size_t size() const
            {
                return 4u;
            }

            void to_json(std::vector<boost::asio::const_buffer>& const_buffer_sequence) const
            {
                static const char text[] = {'n', 'u', 'l', 'l'};

                const_buffer_sequence.push_back(boost::asio::buffer(text));
            }

            type_code code() const
            {
                return null_code;
            }

        };

    } // namespace

    ////////////////
    // class string
    string::string()
        : value(new string_impl(""))
    {
    }

    string::string(const char* s)
        : value(new string_impl(s))
    {
    }

    ///////////////
    // class number
    number::number(int value)
        : value(new number_impl(value))
    {
    }

    number::number(double value)
        : value(new number_impl(value))
    {
    }

    ///////////////
    // class object
    object::object()
        : value(new object_impl())
    {
    }

    void object::add(const string& name, const value& val)
    {
        get_impl<object_impl>().add(name, val);
    }

    ///////////////
    // class array
    array::array()
        : value(new array_impl())
    {
    }

    void array::add(const value& val)
    {
        get_impl<array_impl>().add(val);
    }

    static const boost::shared_ptr<value::impl> single_true(new true_impl());
    static const boost::shared_ptr<value::impl> single_false(new false_impl());
    static const boost::shared_ptr<value::impl> single_null(new null_impl());

    true_val::true_val()
        : value(single_true)
    {
    }

    false_val::false_val()
        : value(single_false)
    {
    }

    null::null() : value(single_null)
    {
    }


    //////////////
    // class value
    bool value::operator < (const value& rhs) const
    {
        const tools::dynamic_type this_type(typeid(*pimpl_));
        const tools::dynamic_type that_type(typeid(*rhs.pimpl_));

        if ( this_type != that_type )
            return this_type < that_type;

        bool result = false;

        struct v : visitor
        {
            v(bool& b, value::impl& i) : result(b), lhs(i) {}
            void visit(const string_impl& rhs) const
            {
                result = static_cast<const string_impl&>(lhs) < rhs;
            }

            void visit(const number_impl&) const {}
            void visit(const object_impl&) const {}
            void visit(const array_impl&) const {}
            void visit(const true_impl&) const {}
            void visit(const false_impl&) const {}
            void visit(const null_impl&) const {}

            bool&           result;
            value::impl&    lhs;
        } const compare(result, *pimpl_);

        rhs.pimpl_->visit(compare);

        return result;
    }

    value::~value()
    {
    }

    std::size_t value::size() const
    {
        return pimpl_->size();
    }

    void value::to_json(std::vector<boost::asio::const_buffer>& const_buffer_sequence) const
    {
        pimpl_->to_json(const_buffer_sequence);
    }

    std::string value::to_json() const
    {
        std::vector<boost::asio::const_buffer> data;
        to_json(data);

        std::string result;
        for ( std::vector<boost::asio::const_buffer>::const_iterator i = data.begin(); i != data.end(); ++i)
        {
            result.append(boost::asio::buffer_cast<const char*>(*i), boost::asio::buffer_size(*i));
        }

        return result;
    }

    value::value(impl* p)
        : pimpl_(p)
    {
    }

    value::value(const boost::shared_ptr<impl>& impl)
        : pimpl_(impl)
    {
    }

    template <class Type>
    Type& value::get_impl()
    {
        return static_cast<Type&>(*pimpl_);
    }

    template <class Type>
    const Type& value::get_impl() const
    {
        return static_cast<const Type&>(*pimpl_);
    }

    std::ostream& operator<<(std::ostream& out, const value& v)
    {
        return out << v.to_json();
    }

    bool operator==(const value& lhs, const value& rhs)
    {
        return !(lhs < rhs) && !(rhs < lhs); 
    }

    bool operator!=(const value& lhs, const value& rhs)
    {
        return !(lhs == rhs);
    }


} // namespace json

