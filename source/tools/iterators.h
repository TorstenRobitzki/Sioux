// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SOURCE_TOOLS_ITERATORS_H
#define SOURCE_TOOLS_ITERATORS_H

#include <iterator>

/// @file tools/iterators.h
/// iterators and iterator related functions

/// @namespace tools
/// tools and helpers
namespace tools {
  
/////////////////////////////////////////////////
///
/// @fn T * begin(T(&t)[S]) 
/// @brief returns a pointer to the first element of an array
template <class T, unsigned S>
T* begin(T(&t)[S])
{
    return &t[0];
}

/////////////////////////////////////////////////
///
/// @fn T* begin(const T(&t)[S])
/// @brief returns a pointer to the first element of an const array
/// @ingroup tools

template <class T, unsigned S>
const T* begin(const T(&t)[S])
{
    return &t[0];
}

/////////////////////////////////////////////////
///
/// @fn T* end(T(&t)[S])
/// @brief returns a pointer behind the last element of an array
/// @ingroup tools

template <class T, unsigned S>
T* end(T(&t)[S])
{
    return &t[S];
}

/////////////////////////////////////////////////
///
/// @fn T* end(const T(&t)[S])
/// @brief returns a pointer behind the last element of an const array
/// @ingroup tools

template <class T, unsigned S>
const T* end(const T(&t)[S])
{
    return &t[S];
}

namespace detail {

// getting the derefenced value_type from an iterator type
template <class Iter>
struct get_iterator_value_type
{
    /// @brieff smart pointers
    template <class U>
    struct dereference {
        typedef typename U::element_type element_type;
    };
    
    /// @brief normal pointers
    template <class U>
    struct dereference<U*>
    {
        typedef U element_type;
    };
    
    typedef typename dereference<typename std::iterator_traits<Iter>::value_type>::element_type value_type;
};
} // namespace detail

/////////////////////////////////////////////////
///
/// @class ptr_iterator_adapter iterators.h "tools/iterators.h"
/// 
/// @brief An adapter for interators to pointer like types
///
/// This is an interator that takes an interator with a pointer like 
/// value_type as a constructor argument and behaves like an iterator
/// with the dereferenced value_type. Thus is might me usefull for 
/// collections of pointers or smart pointers. If smart pointers are
/// used, the smart pointer type have to have a typename 'element_type'
/// like std::auto_ptr<> or boost::shared_ptr<> 

template <class Iter>
class ptr_iterator_adapter : public std::iterator<typename std::iterator_traits<Iter>::iterator_category,
                                                  typename detail::get_iterator_value_type<Iter>::value_type>
{
public:
    typedef std::iterator<typename std::iterator_traits<Iter>::iterator_category,
                          typename detail::get_iterator_value_type<Iter>::value_type> base_class;
                          
    ptr_iterator_adapter() : iter_() {}
    ptr_iterator_adapter(const Iter i) : iter_(i) {}
    bool operator==(const ptr_iterator_adapter& rhs) const { return iter_ == rhs.iter_; }
    bool operator!=(const ptr_iterator_adapter& rhs) const { return iter_ != rhs.iter_; }
    typename base_class::pointer operator->() const { return &**iter_; }
    typename base_class::reference operator*() const { return **iter_; }
    typename base_class::reference operator[](typename base_class::difference_type i) const { return *iter_[i]; }
    ptr_iterator_adapter& operator++() { ++iter_; return *this; }
    ptr_iterator_adapter  operator++(int) { Iter tmp(iter_); ++iter_; return tmp; }
    ptr_iterator_adapter& operator--()    { --iter_; return *this; }
    ptr_iterator_adapter  operator--(int) { Iter tmp(iter_); --iter_; return tmp; }
    ptr_iterator_adapter&  operator+=(typename base_class::difference_type i) { iter_+=i; return *this; }
    ptr_iterator_adapter&  operator-=(typename base_class::difference_type i) { iter_-=i; return *this; }
    typename base_class::difference_type        operator-(const ptr_iterator_adapter& rhs) const { return iter_ - rhs.iter_; }
    bool operator<(const ptr_iterator_adapter& rhs) const  { return iter_ < rhs.iter_; }
    bool operator>(const ptr_iterator_adapter& rhs) const  { return iter_ > rhs.iter_; }
    bool operator<=(const ptr_iterator_adapter& rhs) const { return !(iter_ > rhs.iter_); }
    bool operator>=(const ptr_iterator_adapter& rhs) const { return !(iter_ < rhs.iter_); }
    Iter iter() const { return iter_; }
//private:
    Iter iter_;    
};

template <class Iter>
ptr_iterator_adapter<Iter> operator+(typename ptr_iterator_adapter<Iter>::difference_type lhs, 
                                     const ptr_iterator_adapter<Iter>&                    rhs)
{
    ptr_iterator_adapter<Iter> tmp(rhs);
    tmp += lhs;
    return tmp;
}

template <class Iter>
ptr_iterator_adapter<Iter> operator+(const ptr_iterator_adapter<Iter>&                    lhs, 
                                     typename ptr_iterator_adapter<Iter>::difference_type rhs)
{
    ptr_iterator_adapter<Iter> tmp(lhs);
    tmp += rhs;
    return tmp;
}
                               
template <class Iter> 
ptr_iterator_adapter<Iter> operator-(const ptr_iterator_adapter<Iter>&                    lhs, 
                                     typename ptr_iterator_adapter<Iter>::difference_type rhs)
{
    ptr_iterator_adapter<Iter> tmp(lhs);
    tmp -= rhs;
    return tmp;    
}


} // namespace tools


#endif // include guard


