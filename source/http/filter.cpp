// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "http/filter.h"
#include "http/parser.h"
#include "tools/split.h"

namespace http {
    namespace {
        struct sort_caseless : std::binary_function<tools::substring, tools::substring, bool>
        {
            bool operator()(const tools::substring& lhs, const tools::substring& rhs) const
            {
                return http::strcasecmp(lhs.begin(), lhs.end(), rhs.begin(), rhs.end()) == -1;
            }
        };
    }

    ///////////////////////
    // class filter
    filter::filter(const char* k)
        : values_(k, k + std::strlen(k))
        , index_()
    {
        build_index();
    }

    filter::filter(const tools::substring& k)
        : values_(k.begin(), k.end())
        , index_()
    {
        build_index();
    }

    filter::filter()
        : values_()
        , index_()
    {
    }

    filter::filter(const filter& org)
        : values_(org.values_)
        , index_()
    {
        build_index();
    }

    filter& filter::operator=(const filter& rhs)
    {
        values_ = rhs.values_;
        build_index();

        return *this;
    }

    filter::~filter()
    {
#       ifndef NDEBUG
        std::fill(values_.begin(), values_.end(), 'D');
#       endif

    }

    bool filter::operator()(const tools::substring& key) const
    {
        return std::binary_search(index_.begin(), index_.end(), key, sort_caseless());
    }

    filter& filter::operator+=(const filter& rhs)
    {
        values_.push_back(',');
        values_.insert(values_.end(), rhs.values_.begin(), rhs.values_.end());

        build_index();

        return *this;
    }
        void add_element(const char* begin, const char* end);

    void filter::add_element(const char* begin, const char* end)
    {
        index_.push_back(
            tools::substring(
                http::eat_spaces_and_CRLS(begin, end),
                http::reverse_eat_spaces_and_CRLS(begin, end)
            ));
    }

    void filter::build_index()
    {
        index_.clear();
        tools::substring all(&values_[0], &values_[0] + values_.size());

        for ( tools::substring sub; tools::split(all, ',', sub, all); )
        {
            add_element(sub.begin(), sub.end());
        }

        add_element(all.begin(), all.end());

        std::sort(index_.begin(), index_.end(), sort_caseless());
    }

    filter operator+(const filter& lhs, const filter& rhs)
    {
        filter result(lhs);
        result += rhs;

        return result;
    }
} // namespace http