// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SOURCE_TOOLS_SUBSTRING_H
#define SOURCE_TOOLS_SUBSTRING_H

#include <iterator>
#include <cassert>
#include <algorithm>
#include <ostream>
#include <boost/asio/buffer.hpp>

namespace tools 
{
    /**
     * @brief string class with std::string like interface but without responsibility for memory keeping
     * 
     * The substring class points into a foreign buffer with two iterators. The responsibility for the memory
     * in use is outside the class. That's the reason, why this class has no modifying operations.
     */
    template <class Iterator>
    class basic_substring
    {
    public:
        typedef Iterator iterator, const_iterator;
        typedef typename std::iterator_traits<Iterator>::value_type value_type;

        /**
         * @brief an empty substring
         */
        basic_substring();

        /**
         * @brief a substring starting at begin and ending before end
         */
        basic_substring(const_iterator begin, const_iterator end);

        /**
         * @brief returns true, if the string contains zero elements
         */
        bool empty() const;

        template <class Other>
        bool operator==(const basic_substring<Other>& rhs) const;

        template <class Other>
        bool operator!=(const basic_substring<Other>& rhs) const;

        bool operator==(const value_type* rhs) const;

        bool operator!=(const value_type* rhs) const;

        const_iterator begin() const { return begin_; } 
        const_iterator end() const { return end_; }

        std::size_t size() const;

        /**
         * @brief removes all characters a the beginning of the string, that are equal to the given one
         */
        basic_substring& trim_left(value_type to_be_removed);

        /**
         * @brief removes all characters a the end of the string, that are equal to the given one
         */
        basic_substring& trim_right(value_type to_be_removed);

        /**
         * @brief removes all characters a the end and at the begin of the string, that are equal to the given one
         */
        basic_substring& trim(value_type to_be_removed);

        /**
         * @brief implicit conversion to boost::asio::const_buffer possible
         */
        operator boost::asio::const_buffer() const;
    private:
        const_iterator  begin_;
        const_iterator  end_;
    };

    template <class Iterator>
    inline std::size_t buffer_size(const basic_substring<Iterator>& b)
    {
        return b.size();
    }

    typedef basic_substring<const char*> substring;

    template <class Iterator>
    basic_substring<Iterator>::basic_substring()
        : begin_(Iterator())
        , end_(Iterator())
    {
    }

    template <class Iterator>
    basic_substring<Iterator>::basic_substring(const_iterator begin, const_iterator end)
        : begin_(begin)
        , end_(end)
    {
    }

    template <class Iterator>
    bool basic_substring<Iterator>::empty() const
    {
        return begin_ == end_;
    }

    template <class Iterator>
    template <class Other>
    bool basic_substring<Iterator>::operator==(const basic_substring<Other>& rhs) const
    {
        Other       rbegin = rhs.begin();
        Iterator    lbegin = begin_;
        for ( ; lbegin != end_ && rbegin != rhs.end() && *lbegin == *rbegin; ++lbegin, ++rbegin )
            ;

        return lbegin == end_ && rbegin == rhs.end();
    }

    template <class Iterator>
    template <class Other>
    bool basic_substring<Iterator>::operator!=(const basic_substring<Other>& rhs) const
    {
        return !(*this == rhs);
    }

    template <class Iterator>
    bool basic_substring<Iterator>::operator==(const value_type* rhs) const
    {
        assert(rhs);

        const_iterator i = begin_;
        for ( ; i != end_ && *rhs != value_type() && *i == *rhs; ++i, ++rhs )
            ;

        return i == end_ && *rhs == value_type();
    }

    template <class Iterator>
    bool basic_substring<Iterator>::operator!=(const value_type* rhs) const
    {
        return !(*this == rhs);
    }

    template <class Iterator>
    std::size_t basic_substring<Iterator>::size() const
    {
        return std::distance(begin_, end_);
    }

    template <class Iterator>
    basic_substring<Iterator>& basic_substring<Iterator>::trim_left(value_type to_be_removed)
    {
        for (; begin_ != end_ && *begin_ == to_be_removed; ++begin_)
            ;

        return *this;
    }

    template <class Iterator>
    basic_substring<Iterator>& basic_substring<Iterator>::trim_right(value_type to_be_removed)
    {
        for (; begin_ != end_ && *(end_-1) == to_be_removed; --end_)
            ;

        return *this;
    }

    template <class Iterator>
    basic_substring<Iterator>& basic_substring<Iterator>::trim(value_type to_be_removed)
    {
        return trim_left(to_be_removed).trim_right(to_be_removed);
    }

    template <class Iterator>
    basic_substring<Iterator>::operator boost::asio::const_buffer() const
    {
        return boost::asio::const_buffer(begin_, size() * sizeof (typename std::iterator_traits<Iterator>::value_type));
    }

    template <class Iterator>
    std::ostream& operator<<(std::ostream& out, const basic_substring<Iterator>& text)
    {
        for ( typename basic_substring<Iterator>::const_iterator i = text.begin(), end = text.end(); i != end; ++i )
            out << *i;

        return out;
    }

    template <class Iterator>
    bool operator==(const typename basic_substring<Iterator>::value_type* lhs, const basic_substring<Iterator>& rhs)
    {
        return rhs == lhs;
    }

    template <class Iterator>
    bool operator!=(const typename basic_substring<Iterator>::value_type* lhs, const basic_substring<Iterator>& rhs)
    {
        return rhs != lhs;
    }

} // namespace tools 


#endif 