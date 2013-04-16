#include "asio_mocks/test_io_plan.h"
#include <cassert>

namespace asio_mocks
{

    struct read_items
    {
        read_plan::item             item_;
        boost::function< void() >   func_;

        read_items( const read_plan::item& i ) : item_(i)
        {
        }

        read_items( const boost::function< void() >& f ) : item_(), func_( f )
        {
        }

        bool empty() const
        {
            return func_.empty() && item_.first.empty();
        }
    };

    ////////////////
	// class read_plan::impl
	struct read_plan::impl
	{
		impl()
			: steps_()
			, next_(0)
		{
		}

		read_items fetch_item()
		{
            assert(!steps_.empty());

            read_items result = steps_[next_];

            ++next_;

            if ( next_ == steps_.size() )
                next_ = 0;

            return result;
		}

		item next_read()
	    {
		    read_items result = fetch_item();

	        for ( ; !result.func_.empty(); result = fetch_item() )
	        {
	            result.func_();
	        }

	        return result.item_;
	    }

	    void add( const std::string& s )
	    {
	        if ( !steps_.empty() && steps_.back().empty() )
	        {
	            steps_.back().item_.first = s;
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

	    void execute( const boost::function< void() >& f )
	    {
	        steps_.push_back( f );
	    }

	    std::vector<read_items>                   steps_;
        std::vector<read_items>::size_type        next_;
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

    void read_plan::execute( const boost::function< void() >& f )
    {
        assert( pimpl_.get() );
        return pimpl_->execute( f );
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

    read_plan operator<<( read_plan plan, const boost::function< void() >& f )
    {
        plan.execute( f );

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
            if ( !plan_.empty() && plan_.back().size == 0 && !plan_.back().error_code )
            {
                plan_.back().size = s;
            }
            else
            {
                const item new_item = { s };
                plan_.push_back( new_item );
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
        const item new_item = { 0, delay };
        pimpl_->plan_.push_back( new_item );
    }

    void write_plan::error( const boost::system::error_code& ec )
    {
        const item new_item = { 0, boost::posix_time::time_duration(), ec };
        pimpl_->plan_.push_back( new_item );
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

    write_plan operator<<( write_plan plan, const boost::system::error_code& ec )
    {
        plan.error( ec );

        return plan;
    }

} // asio_mocks


