// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/test_io_plan.h"
#include <cassert>

namespace server
{

namespace test
{

    //////////////////
    // class read_plan 
    read_plan::read_plan()
        : steps_()
        , next_(0)
    {
    }

    read_plan::item read_plan::next_read()
    {
        assert(!steps_.empty());

        const item result = steps_[next_];

        ++next_;
        
        if ( next_ == steps_.size() )
            next_ = 0;

        return result;
    }

    void read_plan::add(const std::string& s)
    {
        if ( !steps_.empty() && steps_.back().first.empty() )
        {
            steps_.back().first = s;   
        }
        else
        {
            steps_.push_back(std::make_pair(s, boost::posix_time::time_duration()));
        }
    }

    void read_plan::delay(const boost::posix_time::time_duration& delay)
    {
        assert(delay > boost::posix_time::time_duration());
        steps_.push_back(std::make_pair(std::string(), delay));
    }

    bool read_plan::empty() const
    {
        return steps_.empty();
    }

    read::read(const std::string& s)
        : data(s)
    {
    }

    delay::delay(const boost::posix_time::time_duration& v)
        : delay_value(v)
    {
    }

    read_plan& operator<<(read_plan& plan, const read& r)
    {
        plan.add(r.data);

        return plan;
    }

    read_plan& operator<<(read_plan& plan, const delay& d)
    {
        plan.delay(d.delay_value);

        return plan;
    }

    write_plan::write_plan()
        : plan_()
        , next_(0)
    {
    }

    write_plan::item write_plan::next_write()
    {
        assert(!plan_.empty());

        const item result = plan_[next_];

        ++next_;
        
        if ( next_ == plan_.size() )
            next_ = 0;

        return result;
    }

    void write_plan::add(const std::size_t& s)
    {
        if ( !plan_.empty() && plan_.back().first == 0 )
        {
            plan_.back().first = s;
        }
        else
        {
            plan_.push_back(std::make_pair(s, boost::posix_time::time_duration()));
        }
    }

    void write_plan::delay(const boost::posix_time::time_duration& delay)
    {
        plan_.push_back(std::make_pair(0, delay));
    }

    bool write_plan::empty() const
    {
        return plan_.empty();
    }

    write::write(std::size_t s)
        : size(s)
    {
    }

    write_plan& operator<<(write_plan& plan, const write& w)
    {
        plan.add(w.size);

        return plan;
    }

    write_plan& operator<<(write_plan& plan, const delay& d)
    {
        plan.delay(d.delay_value);

        return plan;
    }

} // test
} // namespace server


