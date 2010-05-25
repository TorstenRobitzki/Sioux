// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "parser.h"
#include "tools/asstring.h"
#include <boost/regex.hpp>

namespace http {

std::string to_lower(const std::string& s)
{
	std::string result(s);
	to_lower(result);
	
	return result;
}

void to_lower(std::string& s)
{
	to_lower(s.begin(), s.end());
}

void split_url(const std::string& url, std::string& scheme, std::string& authority, std::string& path, std::string& query, std::string& fragment)
{
	// accoding to rfc 3986
	static boost::regex expression("^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?");
	boost::cmatch what;	

	if ( boost::regex_match(url.c_str(), what, expression) )
	{
		scheme		= what[2];
		authority 	= what[4];
		path 		= what[5];
		query		= what[7];
		fragment 	= what[9];
	}
	else
	{
		throw bad_url(url);
	}
	
}

namespace {
	template <class Iter>
	int read_nibble(Iter begin, Iter end) {
		if ( begin == end )
			throw bad_url("Value missing after %");
		
		if ( *begin >= '0' && *begin <= '9' )
			return static_cast<int>(*begin) - static_cast<int>('0');
		else if ( *begin >= 'a' && *begin <= 'f' )
			return static_cast<int>(*begin) - static_cast<int>('a') + 10;
		else if ( *begin >= 'A' && *begin <= 'F' )
			return static_cast<int>(*begin) - static_cast<int>('A') + 10; 

		throw bad_url(tools::as_string(static_cast<int>(*begin)) + " is not a hexdigit.");
	}
}

std::string url_decode(const std::string& s)
{
	std::string result;
	result.reserve(s.size());
	
	for ( std::string::const_iterator i = s.begin(), end = s.end(); i != end; ++i )
	{
		if ( *i != '%' )
		{
			result.push_back(*i);
		}
		else
		{
			int value = read_nibble(++i, end) * 16;
			value += read_nibble(++i, end);
			
			result.push_back(static_cast<char>(value));
		}
	}

	return result;
}

int strcasecmp(const char* begin1, const char* end1, const char* null_terminated_str)
{
    for ( ; begin1 != end1 && *null_terminated_str; ++begin1, ++null_terminated_str)
    {
        const char cl = static_cast<char>(std::tolower(*begin1));
        const char cr = static_cast<char>(std::tolower(*null_terminated_str));

        if ( cl < cr )
            return -1;
        if ( cr < cl )
            return 1;
    }

    if ( *null_terminated_str )
        return -1;

    if ( begin1 != end1 )
        return 1;

    return 0;
}

////////////////////////////////////////////////
// class bad_url
bad_url::bad_url(const std::string& s)
 : std::runtime_error("Bad-Url: " + s)
{

}


} // namespace sioux

