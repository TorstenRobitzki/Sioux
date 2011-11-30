// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/test_io_plan.h"
#include <cassert>

namespace server
{

namespace test
{
	////////////////
	// class read_plan::impl
	struct read_plan::impl
	{
		impl()
			: steps_()
			, next_(0)
		{
		}

		item next_read()
	    {
	        assert(!steps_.empty());

	        const item result = steps_[next_];

	        ++next_;

	        if ( next_ == steps_.size() )
	            next_ = 0;

	        return result;
	    }

	    void add( const std::string& s )
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

	    void delay( const boost::posix_time::time_duration& delay)
	    {
	        assert(delay > boost::posix_time::time_duration());
	        steps_.push_back(std::make_pair(std::string(), delay));
	    }

	    std::vector<item>                   steps_;
        std::vector<item>::size_type        next_;
	};

    //////////////////
    // class read_plan 
    read_plan::read_plan()
        : pimpl_( new impl )
    {
    }

    read_plan::item read_plan::next_read()
    {
    	assert( pimpl_.get() );
    	return pimpl_->next_read();
    }

    void read_plan::add(const std::string& s)
    {
    	assert( pimpl_.get() );
    	return pimpl_->add( s );
    }

    void read_plan::delay(const boost::posix_time::time_duration& delay)
    {
    	assert( pimpl_.get() );
    	return pimpl_->delay( delay );
    }

    bool read_plan::empty() const
    {
        return pimpl_->steps_.empty();
    }

    //////////////
    // class read

    read::read(const std::string& s)
        : data(s)
    {
    }

    read::read(const char* s)
    	: data(s)
    {
    }

    delay::delay(const boost::posix_time::time_duration& v)
        : delay_value(v)
    {
    }

    read_plan operator<<(read_plan plan, const read& r)
    {
        plan.add(r.data);

        return plan;
    }

    read_plan operator<<(read_plan plan, const delay& d)
    {
        plan.delay(d.delay_value);

        return plan;
    }

    read_plan operator<<( read_plan plan, const disconnect_read& )
    {
    	plan.add("");

    	return plan;
    }

    ///////////////////////////
    // class write_plan::impl
    struct write_plan::impl
    {
    	impl()
			: plan_()
			, next_(0)
		{
		}

        item next_write()
        {
            assert(!plan_.empty());

            const item result = plan_[next_];

            ++next_;

            if ( next_ == plan_.size() )
                next_ = 0;

            return result;
        }

        void add(const std::size_t& s)
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

        std::vector<item>               plan_;
        std::vector<item>::size_type    next_;
    };

    /////////////////////
    // class write_plan
    write_plan::write_plan()
        : pimpl_( new impl )
    {
    }

    write_plan::item write_plan::next_write()
    {
    	assert( pimpl_.get() );
    	return pimpl_->next_write();
    }

    void write_plan::add(const std::size_t& s)
    {
    	assert( pimpl_.get() );
    	pimpl_->add( s );
    }

    void write_plan::delay(const boost::posix_time::time_duration& delay )
    {
        pimpl_->plan_.push_back(std::make_pair(0, delay));
    }

    bool write_plan::empty() const
    {
        return pimpl_->plan_.empty();
    }

    ///////////////
    // class write
    write::write(std::size_t s)
        : size(s)
    {
    }

    write_plan operator<<(write_plan plan, const write& w)
    {
        plan.add(w.size);

        return plan;
    }

    write_plan operator<<(write_plan plan, const delay& d)
    {
        plan.delay(d.delay_value);

        return plan;
    }

} // test
} // namespace server


