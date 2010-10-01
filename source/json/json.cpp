// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "json/json.h"
#include "tools/dynamic_type.h"
#include "tools/asstring.h"
#include "tools/iterators.h"
#include "tools/substring.h"
#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <iterator>
#include <cctype>
#include <map>

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

        class impl_visitor;
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

        virtual void visit(const impl_visitor&) const = 0;
        virtual std::size_t size() const = 0;
        virtual void to_json(std::vector<boost::asio::const_buffer>& const_buffer_sequence) const = 0;
        virtual type_code code() const = 0;
        virtual const char* name() const = 0;
    };

    namespace {
        class impl_visitor
        {
        public:
            virtual void visit(const string_impl&) const = 0;
            virtual void visit(const number_impl&) const = 0;
            virtual void visit(const object_impl&) const = 0;
            virtual void visit(const array_impl&) const = 0;
            virtual void visit(const true_impl&) const = 0;
            virtual void visit(const false_impl&) const = 0;
            virtual void visit(const null_impl&) const = 0;

            virtual ~impl_visitor() {}
        };

        bool less_impl(const std::vector<char>& lhs, const std::vector<char>& rhs)
        {
            if ( lhs.size() != rhs.size() )
                return lhs.size() < rhs.size();

            const std::pair<std::vector<char>::const_iterator, std::vector<char>::const_iterator> found =
                std::mismatch(lhs.begin(), lhs.end(), rhs.begin());

            return found.first != lhs.end() && *found.first < *found.second;
        }

        class string_impl : public value::impl
        {
        public:
            explicit string_impl(std::vector<char>& v)
            {
                data_.swap(v);
            }

            explicit string_impl(const char* s) : data_()
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
                return less_impl(data_, rhs.data_);
            }

        private:
            void visit(const impl_visitor& v) const 
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

            const char* name() const 
            {
                return "string";
            }

            std::vector<char>   data_;
        };

        ///////////////////////////////////////
        // number_impl
        class number_impl : public value::impl
        {
        public:
            explicit number_impl(std::vector<char>& buffer)
            {
                data_.swap(buffer);
            }

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

            bool operator<(const number_impl& rhs) const
            {
                return less_impl(data_, rhs.data_);
            }

            int to_int() const
            {
                const tools::basic_substring<std::vector<char>::const_iterator> s(data_.begin(), data_.end());
                return boost::lexical_cast<int>(s);
            }
        private:
            void visit(const impl_visitor& v) const
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

            const char* name() const 
            {
                return "number";
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
                members_.insert(std::make_pair(name, val));
            }

            bool operator<(const object_impl& rhs) const
            {
                if ( members_.size() != rhs.members_.size() )
                    return members_.size() < rhs.members_.size();

                const std::pair<list_t::const_iterator, list_t::const_iterator> found =
                    std::mismatch(members_.begin(), members_.end(), rhs.members_.begin());
    
                return found.first != members_.end() && *found.first < *found.second;
            }

            std::vector<string> keys() const
            {
                std::vector<string> result;
                for ( list_t::const_iterator i = members_.begin(); i != members_.end(); ++i )
                    result.push_back(i->first);

                return result;
            }

            void erase(const string& key)
            {
                members_.erase(key);
            }

            value& at(const string& key)
            {
                const list_t::iterator pos = members_.find(key);

                if ( pos == members_.end() )
                    throw std::out_of_range("object::at() out of range");

                return pos->second;
            }

            const value& at(const string& key) const
            {
                const list_t::const_iterator pos = members_.find(key);

                if ( pos == members_.end() )
                    throw std::out_of_range("object::at() out of range");

                return pos->second;
            }

        private:
            void visit(const impl_visitor& v) const
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

                for ( list_t::const_iterator i = members_.begin(); i != members_.end(); )
                {
                    i->first.to_json(const_buffer_sequence);
                    const_buffer_sequence.push_back(boost::asio::buffer(colon));
                    i->second.to_json(const_buffer_sequence);

                    ++i;
                    if ( i != members_.end() )
                        const_buffer_sequence.push_back(boost::asio::buffer(comma));
                }

                const_buffer_sequence.push_back(boost::asio::buffer(close));
            }

            type_code code() const
            {
                return object_code;
            }

            const char* name() const 
            {
                return "object";
            }

            typedef std::map<string, value> list_t;
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

            void add(const array_impl& v)
            {
                members_.insert(members_.end(), v.members_.begin(), v.members_.end());
            }

            bool operator<(const array_impl& rhs) const
            {
                if ( members_.size() != rhs.members_.size() )
                    return members_.size() < rhs.members_.size();

                const std::pair<list_t::const_iterator, list_t::const_iterator> found =
                    std::mismatch(members_.begin(), members_.end(), rhs.members_.begin());
    
                return found.first != members_.end() && *found.first < *found.second;
            }

            void erase(std::size_t index, std::size_t size)
            {
                if ( index + size > members_.size() ) 
                    throw std::out_of_range("array::erase() out of range");

                members_.erase(members_.begin() + index, members_.begin() + index + size);
            }

            void insert(std::size_t index, const value& v)
            {
                if ( index > members_.size() ) 
                    throw std::out_of_range("array::insert() out of range");

                members_.insert(members_.begin() + index, v);
            }

            typedef std::vector<value> list_t;
            list_t members_;

        private:
            void visit(const impl_visitor& v) const
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

            const char* name() const 
            {
                return "array";
            }
        };

        ////////////////////
        // class false_impl
        class false_impl : public value::impl
        {
        private:
            void visit(const impl_visitor& v) const
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

            const char* name() const 
            {
                return "false_val";
            }

        };

        ///////////////////
        // class true_impl
        class true_impl : public value::impl
        {
        private:
            void visit(const impl_visitor& v) const
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

            const char* name() const 
            {
                return "true_val";
            }
        };

        ///////////////////
        // class null_impl
        class null_impl : public value::impl
        {
        private:
            void visit(const impl_visitor& v) const
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

            const char* name() const 
            {
                return "null";
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

    int number::to_int() const
    {
        return get_impl<number_impl>().to_int();
    }

    ///////////////
    // class object
    object::object()
        : value(new object_impl())
    {
    }

    object& object::add(const string& name, const value& val)
    {
        get_impl<object_impl>().add(name, val);

        return *this;
    }

    std::vector<string> object::keys() const
    {
        return get_impl<object_impl>().keys();
    }

    void object::erase(const string& key)
    {
        get_impl<object_impl>().erase(key);
    }

    value& object::at(const string& key)
    {
        return get_impl<object_impl>().at(key);
    }

    const value& object::at(const string& key) const
    {
        return get_impl<object_impl>().at(key);
    }

    ///////////////
    // class array
    array::array()
        : value(new array_impl())
    {
    }

    array::array(const value& first_value)
        : value(new array_impl())
    {
        add(first_value);
    }

    array::array(impl* p)
        : value(p)
    {
    }

    array array::copy() const
    {
        return array(new array_impl(get_impl<array_impl>()));
    }

    array& array::add(const value& val)
    {
        get_impl<array_impl>().add(val);

        return *this;
    }

    std::size_t array::length() const
    {
        return get_impl<array_impl>().members_.size();
    }

    bool array::empty() const
    {
        return get_impl<array_impl>().members_.empty();
    }

    const value& array::at(std::size_t idx) const
    {
        return get_impl<array_impl>().members_.at(idx);
    }

    value& array::at(std::size_t idx)
    {
        return get_impl<array_impl>().members_.at(idx);
    }

    void array::erase(std::size_t index, std::size_t size)
    {
        get_impl<array_impl>().erase(index, size);
    }

    void array::insert(std::size_t index, const value& v)
    {
         get_impl<array_impl>().insert(index, v);
    }

    array& array::operator+=(const array& rhs)
    {
        get_impl<array_impl>().add(rhs.get_impl<array_impl>());

        return *this;
    }

    array operator+(const array& lhs, const array& rhs)
    {
        array result(lhs);
        result += rhs;

        return result;
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

    /////////////////////////
    // class invalid_cast : public std::runtime_error
    invalid_cast::invalid_cast(const std::string& msg)
     : std::runtime_error(msg)
    {}

    //////////////
    // class value
    void value::visit(visitor& v) const
    {
        struct special : impl_visitor
        {
            special(visitor& v, const value& val) : v_(v), val_(val) 
            {
            }

            void visit(const string_impl&) const
            {
                v_.visit(static_cast<const string&>(val_));
            }

            void visit(const number_impl&) const 
            {
                v_.visit(static_cast<const number&>(val_));
            }

            void visit(const object_impl&) const 
            {
                v_.visit(static_cast<const object&>(val_));
            }

            void visit(const array_impl&) const 
            {
                v_.visit(static_cast<const array&>(val_));
            }

            void visit(const true_impl&) const 
            {
                v_.visit(static_cast<const true_val&>(val_));
            }

            void visit(const false_impl&) const 
            {
                v_.visit(static_cast<const false_val&>(val_));
            }

            void visit(const null_impl&) const 
            {
                v_.visit(static_cast<const null&>(val_));
            }

            visitor&        v_;
            const value&    val_;
        } const execute(v, *this);

        pimpl_->visit(execute);
    }

    bool value::operator < (const value& rhs) const
    {
        const tools::dynamic_type this_type(typeid(*pimpl_));
        const tools::dynamic_type that_type(typeid(*rhs.pimpl_));

        if ( this_type != that_type )
            return this_type < that_type;

        bool result = false;

        struct compare_equal_types : impl_visitor
        {
            compare_equal_types(bool& b, value::impl& i) : result(b), lhs(i) {}

            void visit(const string_impl& rhs) const
            {
                result = static_cast<const string_impl&>(lhs) < rhs;
            }

            void visit(const number_impl& rhs) const 
            {
                result = static_cast<const number_impl&>(lhs) < rhs;
            }

            void visit(const object_impl& rhs) const 
            {
                result = static_cast<const object_impl&>(lhs) < rhs;
            }

            void visit(const array_impl& rhs) const 
            {
                result = static_cast<const array_impl&>(lhs) < rhs;
            }

            // for this three types, the result is false;
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

    namespace {
        template <class A, class B>
        struct equal_types
        {
            static void throw_exception(const char* from, const char* to)
            {
                throw invalid_cast("expected " + std::string(to) + " but got " + std::string(from));
            }
        };

        template <class A>
        struct equal_types<A,A>
        {
            static void throw_exception(const char*, const char*)
            {
            }
        };
    }

#   define CAST_VISITOR_VISIT(target_token) \
        void visit(const target_token&) \
        { \
            equal_types<Target, target_token>::throw_exception(runtime_name_,#target_token); \
        }

    namespace {
        template <class Target>
        struct cast_visitor : visitor
        {
            cast_visitor(const char* implementation_name) : runtime_name_(implementation_name) 
            {
            }

            CAST_VISITOR_VISIT(string)
            CAST_VISITOR_VISIT(number)
            CAST_VISITOR_VISIT(object)
            CAST_VISITOR_VISIT(array)
            CAST_VISITOR_VISIT(true_val)
            CAST_VISITOR_VISIT(false_val)
            CAST_VISITOR_VISIT(null)

            const char* const runtime_name_;
        };
    }

    template <class TargetType>
    TargetType value::upcast() const
    {
        cast_visitor<TargetType> throw_invalid_cast(pimpl_->name());
        visit(throw_invalid_cast);

        return static_cast<const TargetType&>(*this);
    }

    template string     value::upcast<string>() const;
    template number     value::upcast<number>() const;
    template object     value::upcast<object>() const;
    template array      value::upcast<array>() const;
    template true_val   value::upcast<true_val>() const;
    template false_val  value::upcast<false_val>() const;
    template null       value::upcast<null>() const;

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

    /////////////////////
    // class parse_error
    parse_error::parse_error(const std::string& s) : std::runtime_error(s)
    {
    }

    ////////////////
    // class parser
    namespace {
        enum parser_state 
        {
            idle_parsing = 0,
            start_number_parsing = 100,
                sign_parsed,
                pre_dot_parsed,
                leading_zero_parsed,
                dot_parsed,
                post_dot_parsed,
                exponent_parsed,
                exponent_sign_parsed,
                exponent_value_parsed,
            start_object_parsing = 200,
                left_brace_parsed,
                member_name_parsed, 
                member_value_parsed,
            start_array_parsing = 300,
                left_bracket_parsed,
                array_value_parsed,
            start_string_parsing = 400,
                string_parsing,
                reverse_solidus_parsed,
                unicode_marker_parse,
            start_true_parsing = 500,
            start_false_parsing = 600,
            start_null_parsing = 700,
        };

        int main_state(int state)
        {
            return state - (state % 100);
        }
    }

    parser::parser()
    {
        state_.push(idle_parsing);
    }

    static const char* eat_white_space(const char* begin, const char* end)
    {
        for ( ; begin != end && ( *begin == ' ' || *begin == '\t' || *begin == '\n' || *begin == '\r' ); ++begin )
            ;

        return begin;
    }

    bool parser::parse(const char* begin, const char* end)
    {
        assert(!state_.empty());

        for ( ; begin != end && !state_.empty(); )
        {
            switch ( main_state(state_.top()) )
            {
            case idle_parsing:
                begin = eat_white_space(begin, end);

                if ( begin != end )
                {
                    state_.top() = parse_idle(*begin);
                }
                break;
            case start_number_parsing:
                begin = parse_number(begin, end);
                break;
            case start_array_parsing:
                begin = parse_array(begin, end);
                break;
            case start_object_parsing:
                begin = parse_object(begin, end);
                break;
            case start_string_parsing:
                begin = parse_string(begin, end);
                break;
            case start_true_parsing:
            case start_false_parsing:
            case start_null_parsing:
                begin = parse_literal(begin, end);
                break;
            default:
                assert(!"should not happen");
            }
        }

        return state_.empty();
    }

    int parser::parse_idle(char c)
    {
        switch ( c )
        {
            case '{':
                return start_object_parsing;
            case '[':
                return start_array_parsing;
            case '\"':
                return start_string_parsing;
            case 'f':
                return start_false_parsing;
            case 't':
                return start_true_parsing;
            case 'n':
                return start_null_parsing;

            case '-':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                return start_number_parsing;
        }

        throw parse_error("Unexpected character: " + tools::as_string(int(c)));
    }


    static int state_after_digit(int old_state)
    {
        if ( old_state >= exponent_parsed )
        {
            return exponent_value_parsed;
        }
        else if ( old_state >= dot_parsed )
        {
            return post_dot_parsed;
        }

        return pre_dot_parsed;
    }

    static bool is_complete_number(int state)
    {
        return state == pre_dot_parsed 
            || state == leading_zero_parsed
            || state == post_dot_parsed
            || state == exponent_value_parsed;
    }

    const char* parser::parse_number(const char* begin, const char* end)
    {
        assert(!state_.empty());
        bool stop = false;

        for ( ; begin != end && !stop; )
        {
            switch ( *begin )
            {
            case '-':
            case '+':
                if ( state_.top() > start_number_parsing && state_.top() != exponent_parsed )
                    throw parse_error("unexpected sign");

                state_.top() = state_.top() == exponent_parsed
                    ? exponent_sign_parsed
                    : sign_parsed;
                break;
            case '.':
                if ( state_.top() != pre_dot_parsed && state_.top() != leading_zero_parsed )
                    throw parse_error("unexpected dot(.)");

                state_.top() = dot_parsed;
                break;
            case '0':
                if ( state_.top() != sign_parsed && state_.top() != start_number_parsing && state_.top() != pre_dot_parsed
                    && state_.top() != dot_parsed && state_.top() != post_dot_parsed && state_.top() < exponent_parsed )
                {
                    throw parse_error("unexpected 0");
                }

                state_.top() = state_after_digit(state_.top());
                break;
            case 'e':
            case 'E':
                if ( state_.top() != leading_zero_parsed && state_.top() != pre_dot_parsed && state_.top() != post_dot_parsed )
                    throw parse_error("unexpected exponent");

                state_.top() = exponent_parsed;
                break;
            default :
                if (isdigit(*begin))
                {
                    state_.top() = state_after_digit(state_.top());
                }
                else if ( is_complete_number(state_.top()) )
                {
                    stop = true;
                }
                else
                {
                    throw parse_error("incomplete number");
                }
            }

            if ( !stop )
            {
                buffer_.push_back(*begin);
                ++begin;
            }
        }

        if ( stop )
        {
            value_parsed(value(new number_impl(buffer_)));
        }

        return begin;
    }

    const char* parser::parse_array(const char* begin, const char* end)
    {
        if ( state_.top() == start_array_parsing )
        {
            assert(begin != end && *begin == '[');

            state_.top() = left_bracket_parsed;             
            result_.push(array());

            ++begin;
        }
        else if ( state_.top() == left_bracket_parsed )
        {
            begin = eat_white_space(begin, end);

            if ( begin != end )
            {
                if ( *begin == ']' )
                {
                    state_.pop();
                    ++begin;
                }
                else
                {
                    state_.top() = array_value_parsed;
                    state_.push(idle_parsing);
                }
            }
        }
        else
        {
            assert(state_.top() == array_value_parsed);

            begin = eat_white_space(begin, end);

            if ( begin != end )
            {
                if ( *begin == ',' )
                {
                    state_.top() = array_value_parsed;
                    state_.push(idle_parsing);
                }
                else if ( *begin == ']' )
                {
                    state_.pop();
                }
                else
                {
                    throw parse_error("Unexpected char while parsing array: " + tools::as_string(int(*begin)));
                }

                ++begin;
                const value ele = result_.top();
                result_.pop();
                static_cast<array&>(result_.top()).add(ele);
            }
        }

        return begin;
    }

    const char* parser::parse_object(const char* begin, const char* end)
    {
        if ( state_.top() == start_object_parsing )
        {
            assert(begin != end && *begin == '{');

            state_.top() = left_brace_parsed;             
            result_.push(object());

            ++begin;
        }
        else if ( state_.top() == left_brace_parsed )
        {
            begin = eat_white_space(begin, end);

            if ( begin != end )
            {
                if ( *begin == '}' )
                {
                    ++begin;
                    state_.pop();
                }
                else if ( *begin == '\"' )
                {
                    state_.top() = member_name_parsed;
                    state_.push(start_string_parsing);
                }
                else
                {
                    throw parse_error("Object pair must begin with a string");
                }
            }
        }
        else if ( state_.top() == member_name_parsed )
        {
            begin = eat_white_space(begin, end);

            if ( begin != end )
            {
                if ( *begin != ':' )
                    throw parse_error("colon expected");

                state_.top() = member_value_parsed;
                state_.push(idle_parsing);

                ++begin;
            }
        }
        else
        {
            assert(state_.top() == member_value_parsed);

            begin = eat_white_space(begin, end);

            if ( begin != end )
            {
                if ( *begin == ',' )
                   ++begin;

                state_.top() = left_brace_parsed;

                value  val  = result_.top();
                result_.pop();
                string name = static_cast<string&>(result_.top());
                result_.pop();

                static_cast<object&>(result_.top()).add(name, val);
            }
        }

        return begin;
    }

    const char* parser::parse_string(const char* begin, const char* end)
    {
        bool stop = false;

        for ( ; begin != end && !stop; )
        {
            switch ( state_.top() )
            {
            case start_string_parsing: 
                assert(*begin == '\"');
                assert(buffer_.empty());

                state_.top() = string_parsing;

                buffer_.push_back(*begin);
                ++begin;
                break;
            case string_parsing:
                {
                    const char* p = begin;
                    for ( ; p != end && *p != '\"' && *p != '\\'; ++p )
                        ;

                    buffer_.insert(buffer_.end(), begin, p);
                    begin = p;

                    if ( begin != end )
                    {
                        buffer_.push_back(*begin);
                        if ( *begin == '\"' )
                        {
                            value_parsed(value(new string_impl(buffer_)));
                            stop = true;
                        }
                        else 
                        {
                            assert( *begin == '\\' );
                            state_.top() = reverse_solidus_parsed;
                        }

                        ++begin;
                    }
                }
                break;
            case reverse_solidus_parsed:
                {
                    if ( *begin == 'u' )
                    {
                        state_.top() = unicode_marker_parse;
                    }
                    else
                    {
                        static const char escapeable_characters[] = { '\"', '\\', '/', 'b', 'f', 'n', 'r', 't'};

                        if ( std::find(tools::begin(escapeable_characters), tools::end(escapeable_characters), *begin)
                             == tools::end(escapeable_characters) )
                        {
                            throw parse_error("Unexpected escaped char: " + tools::as_string(int(*begin)));
                        }

                        state_.top() = string_parsing;
                    }

                    buffer_.push_back(*begin);
                    ++begin;
                }
                break;
            default:
                {
                    const int missing_hexdigits = 4 - state_.top() + unicode_marker_parse;

                    assert(missing_hexdigits > 0 && missing_hexdigits <= 4);
                    
                    if (!std::isxdigit(*begin))
                        throw parse_error("Hex-Digit expected.");

                    buffer_.push_back(*begin);
                    ++begin;

                    ++state_.top();

                    if ( state_.top() - unicode_marker_parse == 4 )
                        state_.top() = string_parsing;
                }
            }
        }

        return begin;
    }

    const char* parser::parse_literal(const char* begin, const char* end)
    {
        static const char* literals[] = {"true", "false", "null"};
        static const value values[]   = {true_val(), false_val(), null()};

        const int literal = (state_.top() - start_true_parsing) / 100;

        for ( const char* l = &literals[literal][state_.top() % 100]; 
            begin != end && *l != 0; ++begin, ++l )
        {
            if ( *begin != *l )
            {
                throw parse_error("invalid json literal");
            }

            ++state_.top();
        }

        if ( begin != end )
            value_parsed(values[literal]);

        return begin;
    }

    void parser::value_parsed(const value& v)
    {
        state_.pop();
        result_.push(v);
    }

    value parser::result() const   
    {
        assert(state_.empty());
        assert(result_.size() == 1);

        return result_.top();
    }

    void parser::flush()
    {
        // still parsing a number
        if ( !state_.empty() )
        {
            if ( !is_complete_number(state_.top()) )
                throw parse_error("incomplete json number");

            value_parsed(value(new number_impl(buffer_)));
        }

        if ( !state_.empty() || result_.size() != 1 )
            throw parse_error("incomplete json expression");
    }

    value parse(const std::string text)
    {
        return parse(text.begin(), text.end());
    }

} // namespace json

