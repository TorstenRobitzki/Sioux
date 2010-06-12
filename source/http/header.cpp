// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "http/header.h"
#include "http/parser.h"
#include "tools/split.h"
#include <algorithm>
#include <functional>
#include <cstring>
#include <cassert>

namespace http {

    //////////////////
    // struct header
    header::header(const tools::substring& all, const tools::substring& name, const tools::substring& value)
        : all_(all)
        , name_(name)
        , value_(value)
    {
    }
    
    header::header()
        : all_()
        , name_()
        , value_()
    {
    }

    tools::substring header::name() const
    {
        return name_;
    }

    tools::substring header::value() const
    {
        return value_;
    }

    tools::substring header::all() const
    {
        return all_;
    }

    bool header::parse(const char* begin, const char* end)
    {
        const char* const name  = http::eat_spaces_and_CRLS(begin, end);
        const char* const colon = std::find(name, end, ':');

        if ( name != begin || colon == end )
            return false;

        const char* const value = http::eat_spaces_and_CRLS(colon+1, end);

        all_    = tools::substring(begin, end);
        name_   = tools::substring(name, http::reverse_eat_spaces(name, colon));
        value_  = tools::substring(value, http::reverse_eat_spaces_and_CRLS(value, end));

        return true;
    }

    void header::add_value_line(const char* new_end)
    {
        all_    = tools::substring(all_.begin(), new_end);
        value_  = tools::substring(value_.begin(), http::reverse_eat_spaces_and_CRLS(value_.begin(), new_end));
    }

    tools::substring::iterator  header::begin() const
    {
        return all_.begin();
    }

    tools::substring::iterator  header::end() const
    {
        return all_.end();
    }

} // namespace http



