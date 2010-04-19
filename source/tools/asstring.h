// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SOURCE_TOOLS_ASSTRING_H
#define SOURCE_TOOLS_ASSTRING_H

#include <string>
#include <sstream>

/// @file tools/asstring.h
/// std::string asString<T>()
/// @ingroup tools

/// @namespace ritter::tools
/// @ingroup tools
namespace tools {

/*!
 * @defgroup tools tools, helpers and miscellanous
 */

/////////////////////////////////////////////////
///
/// @brief function for converting to std::string
/// @ingroup tools
///
/// Uses std::ostringstream and operator<< to convert
/// t to a string.
template <class T>
std::string as_string(const T& t)
{
  std::ostringstream s;
  s << t;
  return s.str();
}

} // namespace tools

#endif // includeguard
