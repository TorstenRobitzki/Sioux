// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SOURCE_TOOLS_DYNAMIC_TYPE_H
#define SOURCE_TOOLS_DYNAMIC_TYPE_H

#include <typeinfo>

/// @file tools/dynamic_type.h
/// class dynamic_type and other helpers
/// @ingroup tools

namespace tools {

/////////////////////////////////////////////////////////////////////////
/// 
/// @class dynamic_type
///
/// @brief Wrapper around std::type_info 
/// @ingroup tools
///
/// Responsibility : 
///  - Wrapper around type_info, to enable the storeing
///    of runtime type informations
///
class dynamic_type 
{
public:
    dynamic_type(const std::type_info& t) : ti_(&t) {}

    bool operator==(const dynamic_type& rhs) const
    {
        return *ti_ == *rhs.ti_;
    }

    bool operator!=(const dynamic_type& rhs) const
    {
        return !(*this == rhs);
    }
    
    bool operator < (const dynamic_type& rhs) const
    {
        return ti_->before(*rhs.ti_);
    }

    const std::type_info& type() const {return *ti_;}
private:
    const std::type_info* ti_;
};

} // namespace tools 

#endif // include guard
