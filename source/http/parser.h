// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SOURCE_HTTP_PARSER_H
#define SOURCE_HTTP_PARSER_H

#include <string>
#include <algorithm>
#include <locale>
#include <stdexcept>
#include <limits>
#include <cctype>

namespace http {

const char CR = '\r';
const char LS = '\n';
const char SP = ' ';
const char HT = '\t';

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

template <class Iter>
Iter eat_spaces(Iter begin, Iter end) {
	for (; begin != end && is_space(*begin); ++begin )
	;
	
	return begin;
}

template <class Iter>
Iter reverse_eat_spaces(Iter begin, Iter end) {
	for (; end != begin && is_space(*(end-1)); --end )
	;
	
	return end;
}

template <class Iter>
Iter eat_spaces_and_CRLS(Iter begin, Iter end) {
	for (; begin != end && is_space_or_CRLS(*begin) ; ++begin )
	;
	
	return begin;
}

template <class Iter>
Iter reverse_eat_spaces_and_CRLS(Iter begin, Iter end) {
	for (; end != begin && is_space_or_CRLS(*(end-1)) ; --end)
	;
	
	return end;
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

class bad_url : public std::runtime_error {
public:
	explicit bad_url(const std::string&);
};

// Splits an url into pieces
void split_url(const std::string& url, std::string& scheme, std::string& authority, std::string& path, std::string& query, std::string& fragment);

// decodes all encoded characters
std::string url_decode(const std::string&);

template <class Iter>
bool parse_number(Iter begin, Iter end, unsigned& r)
{
    if ( begin == end ) 
        return false;

    unsigned result = 0;

    for ( ; begin != end; ++begin )
    {
        if ( !std::isdigit(*begin) )
            return false;

        if ( result > std::numeric_limits<unsigned>::max() / 10 )
            return false;

        result = 10 * result + *begin - '0';
    }

    r = result;
    return true;
}

/**
 * @brief compares a string given by (begin1, end1] with the null terminated string null_terminated_str
 *        case insensitiv. 
 * 
 * If the first string is less than the second, the function returns -1 if both are equal, the function returns
 * 0 and if the second string ist greater, the function returns 1.
 */
int strcasecmp(const char* begin1, const char* end1, const char* null_terminated_str);

int strcasecmp(const char* begin1, const char* end1, const char* begin2, const char* end2);

} // namespace http

#endif // include guard

