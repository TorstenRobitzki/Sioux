// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SOURCE_HTTP_PARSER_H
#define SOURCE_HTTP_PARSER_H

#include <string>
#include <algorithm>
#include <locale>
#include <stdexcept>

namespace http {

const char CR = 13;
const char LS = 10;
const char SP = 32;
const char HT = 34;

// finds a CRLS and returns an Iterator pointing to the CRLS
template <class Iter>
Iter find_CRLS(Iter begin, Iter end) {
	
	Iter cr_pos = std::find(begin, end, CR);
	
	while ( cr_pos != end )
	{
		if ( cr_pos+1 != end && *(cr_pos+1) == LS )				
			break;
		
		cr_pos = std::find(cr_pos+1, end, CR);
	}
	
	return cr_pos;
}

inline bool is_space(char c) {
	return c == SP || c == HT;
}

inline bool is_space_or_CRLS(char c) {
	return c == SP || c == HT || c == LS || c == CR;
}

inline bool is_separator(char c) {
	switch (c)
	{
	case '(': case ')': case '<': case '>': case '@':
	case ',': case ';': case ':': case '\\': case '"':
	case '/': case '[': case ']': case '?': case '=':
	case '{': case '}': case ' ': case '\t':
		return true;
	default:
		return false;
	}
}

inline bool is_CTL(char c) {
	return c == 127 || c >= 0 && c <= 31;
}

template <class Iter1, class Iter2>
Iter1 eat_spaces(Iter1 begin, Iter2 end) {
	for (; begin != end && is_space(*begin); ++begin )
	;
	
	return begin;
}

template <class Iter1, class Iter2>
Iter1 eat_spaces_and_CRLS(Iter1 begin, Iter2 end) {
	for (; begin != end && is_space_or_CRLS(*begin) ; ++begin )
	;
	
	return begin;
}

template <class Iter>
Iter find_space(Iter begin, Iter end) {
	for (; begin != end && !is_space(*begin); ++begin)
	;
	
	return begin;
}

template <class Iter1, class Iter2>
Iter1 read_token(Iter1 begin, Iter2 end)
{
	for ( ; begin != end && !is_separator(*begin) && !is_CTL(*begin); ++begin)
	;
	
	return begin;	
}

template <class Iter1, class Iter2>
void to_lower(Iter1 begin, Iter2 end) {
	for (; begin != end; ++begin )
		*begin = std::tolower(*begin, std::locale::classic());
}

template <class Iter1, class Iter2>
std::string stripe(Iter1 begin, Iter2 end) 
{
	begin = eat_spaces_and_CRLS(begin, end);
	
	while ( begin != end && is_space_or_CRLS(*(end-1)) )
		--end;
	
	return std::string(begin, end);
}

std::string to_lower(const std::string&);
void        to_lower(std::string&);

class bad_url : std::runtime_error {
public:
	explicit bad_url(const std::string&);
};

// Splits an url into pieces
void split_url(const std::string& url, std::string& scheme, std::string& authority, std::string& path, std::string& query, std::string& fragment);

// decodes all encoded characters
std::string url_decode(const std::string&);

template <class Iter>
bool parse_version_number(Iter begin, Iter end, unsigned& r)
{
    if ( begin == end ) 
        return false;

    unsigned result = 0;

    for ( ; begin != end; ++begin )
    {
        if ( *begin < '0' || *begin > '9' )
            return false;

        if ( result > std::numeric_limits<unsigned>::max() / 10 )
            return false;

        result = 10 * result + *begin - '0';
    }

    r = result;
    return true;
}

} // namespace http

#endif // include guard

