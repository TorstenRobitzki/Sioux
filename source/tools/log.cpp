// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "tools/log.h"
#include "tools/iterators.h"
#include "tools/dynamic_type.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/thread.hpp>
#include <ostream>
#include <istream>
#include <vector>
#include <cstring>
#include <string>
#include <map>
#include <algorithm>
#include <cassert>
#include <deque>

#ifdef min
#   undef min
#endif 

namespace logging {

static const char* level_names[] = {
    "fatal", "error", "warning", "info", "main", "detail", "debug", "all" };


std::ostream& operator<<(std::ostream& out, log_level level)
{
    const int index = static_cast<int>(level);

    if ( tools::end(level_names) - tools::begin(level_names) > index )
        return out << level_names[index];

    return out << "invalid value for log_level: " << index;
}

std::istream& operator>>(std::istream& input, log_level& level)
{
    std::string text;
    input >> text;

    log_level   result = log_level();
    bool        found = false;
    bool        error = false;

    std::vector<log_level> results;

    for ( const char** i = tools::begin(level_names); i != tools::end(level_names) && !error; ++i )
    {
        const std::size_t size      = std::strlen(*i);
        const std::size_t comp_size = std::min(text.size(), size);

        if ( text.size() <= size && text.compare(0, comp_size, *i, comp_size) == 0 )
        {
            error  = found;
            found  = true;
            result = static_cast<log_level>(i - tools::begin(level_names));
        }
    }

    if ( error || !found )
        input.setstate(std::ios_base::badbit);
    else
        level = result;

    return input;
}

namespace {
    // Wrapper around a streambuffer or a shared ptr to stream buffer
    class output_device
    {
    public:
        virtual void write_line(const std::string& line) = 0;
        virtual bool is_this_yours(std::streambuf* buf) = 0;
        virtual ~output_device() {}
    };

    // the log context, that is used, when no context is given.
    struct default_context : context {} def_context;

    // the level that is used, when no level is set for a context
    const log_level   default_level      = info;

    // the maximum number of messages that are queued
    const std::size_t maximum_queue_size = 20;

    class impl 
    {
    public:
        impl()
         : ios_slot_(std::ios_base::xalloc())
         , mutex_()
         , condition_()
         , shutdown_(false)
         , levels_()
         , queue_()
         , overflow_()
         , write_thread_(&impl::write_messages, this)
        {
        }

        ~impl()
        {
            {
                boost::mutex::scoped_lock lock(mutex_);
                shutdown_ = true;
            }

            condition_.notify_all();
            write_thread_.join();
        }

        void add_context(std::ostream& stream, const std::type_info* context)
        {
            stream.pword(ios_slot_) = const_cast<std::type_info*>(context);
        }

        void add_message(const context& c, log_level severity, const std::string& message)
        {
            add_message(tools::dynamic_type(typeid(c)), severity, message);
        }

        void add_message(std::ostringstream& buffer, log_level severity)
        {
            const std::type_info* context = static_cast<const std::type_info*>(buffer.pword(ios_slot_));

            if ( context )
            {
                add_message(tools::dynamic_type(*context), severity, buffer.str());
            }
            else
            {
                add_message(tools::dynamic_type(typeid(default_context)), severity, buffer.str());
            }
        }
    
        void set_level(const context& c, log_level level)
        {
            boost::mutex::scoped_lock lock(mutex_);
            levels_[tools::dynamic_type(typeid(c))] = level;
        }
    
        void add_output(const boost::shared_ptr<output_device>& output)
        {
            boost::mutex::scoped_lock lock(mutex_);
            outputs_.push_back(output);
        }

        void remove_output(std::streambuf* output)
        {
            boost::mutex::scoped_lock lock(mutex_);
            output_list_t::const_iterator pos = outputs_.begin();

            for (; pos != outputs_.end() && !(*pos)->is_this_yours(output); ++pos )
                ;

            if ( pos != outputs_.end() )
                outputs_.erase(pos);
        }

    private:
        void add_message(const tools::dynamic_type& context, log_level severity, const std::string& message)
        {
            boost::mutex::scoped_lock lock(mutex_);

            // lookup the level for the context
            level_map_t::const_iterator level_pos = levels_.find(context);
            const log_level             level     = level_pos == levels_.end() ? default_level : level_pos->second;

            if ( level >= severity )
            {
                if ( queue_.size() == maximum_queue_size )
                {
                    overflow_ = true;
                }
                else
                {
                    if ( overflow_ )
                    {
                        queue_.push_back("..." + message);
                        overflow_ = false;
                    }
                    else
                    {
                        queue_.push_back(message);
                    }

                    condition_.notify_one();
                }
            }
        }

        void write_message(const std::string& msg)
        {
            for ( output_list_t::const_iterator i = outputs_.begin(); i != outputs_.end(); )
            {
                bool remove = true;
                try
                {
                    (*i)->write_line(msg);
                    remove = false;
                }
                catch (...)
                {
                }

                if ( remove )
                    outputs_.erase(i);
                else
                    ++i;
            }
        }

        void write_messages()
        {
            for ( ;; )
            {
                boost::mutex::scoped_lock lock(mutex_);
                while ( !shutdown_ && queue_.empty() )
                    condition_.wait(lock);

                if ( shutdown_ )
                    return;
                
                write_message(queue_.front());
                queue_.pop_front();
            }
        }

        const int                   ios_slot_;

        mutable boost::mutex        mutex_;
        boost::condition_variable   condition_;
        bool                        shutdown_;

        typedef std::map<tools::dynamic_type, log_level> level_map_t;
        level_map_t                 levels_;
        
        typedef std::vector<boost::shared_ptr<output_device> > output_list_t;
        output_list_t               outputs_;
        std::deque<std::string>     queue_;
        bool                        overflow_;

        boost::thread               write_thread_;
    } *pimpl = 0;

    unsigned references = 0;

} // namespace 

namespace details
{
    init_log::init_log()
    {
        if ( references == 0 )
        {
            ++references;
            assert(pimpl == 0);
            pimpl = new impl();
        }
    }

    init_log::~init_log()
    {
        if ( --references == 0 )
            delete pimpl;
    }
}

std::ostream& operator<<(std::ostream& output, const context& c)
{
    pimpl->add_context(output, &typeid(c));
    return output;
}

void add_message(const context& c, log_level severity, const std::string& message)
{
    pimpl->add_message(c, severity, message);
}

void add_message(log_level severity, const std::string& message)
{
    pimpl->add_message(def_context, severity, message);
}

void add_message(std::ostringstream& buffer, log_level severity)
{
    pimpl->add_message(buffer, severity);
}

void set_level(const context& c, log_level level)
{
    pimpl->set_level(c, level);
}

void set_level(log_level level)
{
    pimpl->set_level(def_context, level);
}

namespace {

    class shared_buffer : public output_device
    {
    public:
        explicit shared_buffer(const boost::shared_ptr<std::streambuf>& output)
         : output_(output)
         , stream_(output.get())
        {
        }
    private:
        virtual void write_line(const std::string& line)
        {           
            stream_ << line << std::endl;
        }

        virtual bool is_this_yours(std::streambuf* buf)
        {
            return buf == output_.get();
        }
    
        const boost::shared_ptr<std::streambuf> output_;
        std::ostream                            stream_;
    };

    class referenced_buffer : public output_device
    {
    public:
        explicit referenced_buffer(std::streambuf& b)
         : buffer_(b)
         , stream_(&buffer_)
        {
        }
    private:
        virtual void write_line(const std::string& line)
        {           
            stream_ << line << std::endl;
        }
    
        virtual bool is_this_yours(std::streambuf* buf)
        {
            return &buffer_ == buf;
        }

        std::streambuf&                         buffer_;
        std::ostream                            stream_;
    };
}

void add_output(const boost::shared_ptr<std::streambuf>& output)
{
    pimpl->add_output(boost::shared_ptr<output_device>(new shared_buffer(output)));
}

void add_output(std::streambuf& output)
{
    pimpl->add_output(boost::shared_ptr<output_device>(new referenced_buffer(output)));
}

void add_output(std::ostream& output)
{
    assert(output.rdbuf());
    add_output(*output.rdbuf());
}

void remove_output(const boost::shared_ptr<std::streambuf>& output)
{
    assert(output.get());
    pimpl->remove_output(output.get());
}

void remove_output(std::streambuf& output)
{
    pimpl->remove_output(&output);
}

void remove_output(std::ostream& output)
{
    assert(output.rdbuf());
    pimpl->remove_output(output.rdbuf());
}

} // namespace log

