// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef RITTER_INCLUDE_TOOLS_MEM_TOOLS_H
#define RITTER_INCLUDE_TOOLS_MEM_TOOLS_H

#include <memory>

namespace tools {

/////////////////////////////////////////////////
///
/// @brief exception save push_back 
///
/// Tries to insert the pointer ptr into the container c by
/// calling c.push_back(ptr). If this fails the function 
/// delete the pointer ptr.
///
template <class T, class Cont>
void save_push_back(T* ptr, Cont& c)
{
    std::auto_ptr<T> save(ptr);
    c.push_back(ptr);
    save.release();
}

////////////////////////////////////////////////
///
/// @class ptr_container_guard mem_tools.h "tools/mem_tools.h"
/// @brief Guards dynamic allocated objects contained by a sequence container.
///
/// This little guard takes a reference to a container containing pointers to
/// dynamically allocated objects. If the function dismiss() is not been called until
/// the guard goes out of scope, the d'tor of ptr_container_guard, will call
/// delete on all elements of the container passed to the c'tor.

template <class C>
class ptr_container_guard
{
public:
    ptr_container_guard(const C& c) : cont_(c), dismissed_(false) {}
    ~ptr_container_guard()
    {
        if ( !dismissed_ )
        {
            for ( typename C::const_iterator i = cont_.begin(); i != cont_.end(); ++i )
              delete *i;
        }
    }
    void dismiss()
    {
        dismissed_ = true;
    }
private:
    // not implemented
    ptr_container_guard(const ptr_container_guard&);
    ptr_container_guard& operator=(const ptr_container_guard&);
    
    const C& cont_;
    bool  dismissed_;
};

} // namespace tools


#endif

