// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "unittest++/UnitTest++.h"
#include "tools/log.h"
#include "tools/asstring.h"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <sstream>

TEST(level_output)
{
    CHECK(tools::as_string(logging::fatal) == "fatal");
	CHECK(tools::as_string(logging::error) == "error");
	CHECK(tools::as_string(logging::warning) == "warning");
	CHECK(tools::as_string(logging::info) == "info");
	CHECK(tools::as_string(logging::main) == "main");
	CHECK(tools::as_string(logging::detail) == "detail");
	CHECK(tools::as_string(logging::debug) == "debug");
	CHECK(tools::as_string(logging::all) == "all");
}

namespace 
{
    logging::log_level to_level(const char* text)
    {
        std::stringstream input(text);
        logging::log_level result;
        input >> result;

        if ( input.bad() )
            throw std::runtime_error("not a level");

        return result;
    }
}

TEST(level_input)
{
    CHECK(to_level("fatal") == logging::fatal);
    CHECK(to_level("f") == logging::fatal);
    CHECK(to_level("fata") == logging::fatal);
    CHECK_THROW(to_level("fafa"), std::runtime_error);

    CHECK(to_level("error") == logging::error);
    CHECK(to_level("e") == logging::error);
    CHECK(to_level("err") == logging::error);
    CHECK_THROW(to_level("ror"), std::runtime_error);

    CHECK(to_level("warning") == logging::warning);
    CHECK(to_level("w") == logging::warning);
    CHECK(to_level("war") == logging::warning);

    CHECK(to_level("info") == logging::info);
    CHECK(to_level("i") == logging::info);
    CHECK(to_level("in") == logging::info);

    CHECK(to_level("main") == logging::main);
    CHECK(to_level("m") == logging::main);
    CHECK(to_level("ma") == logging::main);

    CHECK(to_level("detail") == logging::detail);
    CHECK(to_level("det") == logging::detail);
    CHECK_THROW(to_level("d"), std::runtime_error);
    CHECK_THROW(to_level("de"), std::runtime_error);

    CHECK(to_level("debug") == logging::debug);
    CHECK(to_level("deb") == logging::debug);
    CHECK(to_level("debu") == logging::debug);

    CHECK(to_level("all") == logging::all);
    CHECK(to_level("a") == logging::all);
}


namespace {
    struct context1_t : logging::context {} context1;
    struct context2_t : logging::context {} context2;

    void sleep()
    {
        boost::posix_time::ptime now(boost::posix_time::second_clock::universal_time());
        boost::thread::sleep(now + boost::posix_time::millisec(20));

    }

    class test_buffer : public std::streambuf
    {
    public:
        bool test_output(const char* t)
        {
            std::string text;
            {
                boost::mutex::scoped_lock lock(mutex_);

                while ( buffer_.empty() || *(buffer_.end()-1) != '\n' )
                    condition_.wait(lock);
                
                text.swap(buffer_);
            }

            return text.find(t) != std::string::npos;
        }

        bool no_output()
        {
            sleep();
            std::string text;
            {
                boost::mutex::scoped_lock lock(mutex_);
                text.swap(buffer_);
            }

            return text.empty();

        }
    private:
        virtual int_type overflow(int_type c)
        {
            {
                boost::mutex::scoped_lock lock(mutex_);
                buffer_.push_back(c);
            }

            if ( c == '\n' )
                condition_.notify_one();

            return traits_type::not_eof(c);
        }

        boost::mutex                mutex_;
        boost::condition_variable   condition_;
        std::string                 buffer_;
    };

}

TEST(test_set_level)
{
    test_buffer out;
    logging::add_output(out);

    CHECK(out.no_output());
    add_message(logging::info, "hallo");
    CHECK(out.test_output("hallo"));

    add_message(logging::error, "error");
    CHECK(out.test_output("error"));

    add_message(logging::main, "main");
    CHECK(out.no_output());

    // switch level to be very low
    logging::set_level(logging::all);
    add_message(logging::debug, "debug");
    CHECK(out.test_output("debug"));

    add_message(logging::detail, "detail");
    CHECK(out.test_output("detail"));

    // switch level to be very high
    logging::set_level(logging::fatal);
    add_message(logging::main, "main");
    CHECK(out.no_output());
    add_message(logging::error, "error");
    CHECK(out.no_output());        
    add_message(logging::fatal, "fatal");
    CHECK(out.test_output("fatal"));

    logging::set_level(logging::info);
    logging::remove_output(out);
}

TEST(test_context)
{
    test_buffer out;
    logging::add_output(out);

    add_message(context1, logging::info, "hallo");
    CHECK(out.test_output("hallo"));

    add_message(context2, logging::info, "hallo2");
    CHECK(out.test_output("hallo2"));

    logging::set_level(context1, logging::error);
    add_message(context1, logging::info, "hallo");
    CHECK(out.no_output());        

    add_message(context2, logging::info, "hallo2");
    CHECK(out.test_output("hallo2"));

    logging::set_level(context2, logging::warning);
    add_message(context1, logging::info, "hallo");
    CHECK(out.no_output());        

    add_message(context2, logging::info, "hallo2");
    CHECK(out.no_output());        

    add_message(context1, logging::warning, "hallo");
    CHECK(out.no_output());        

    add_message(context2, logging::warning, "hallo2");
    CHECK(out.test_output("hallo2"));

    logging::remove_output(out);
}

TEST(add_context_to_stream)
{
    struct my_context_t : logging::context {} my_context;

    test_buffer out;
    logging::add_output(out);


    add_message(my_context, logging::detail, "foobar");
    CHECK(out.no_output());

    logging::set_level(my_context, logging::detail);
    add_message(my_context, logging::detail, "foobar");
    CHECK(out.test_output("foobar"));

    std::ostringstream stream;
    stream << my_context << "Hallo Welt";
    logging::add_message(stream, logging::detail);
    CHECK(out.test_output("Hallo Welt"));

    logging::remove_output(out);
}

TEST(multiple_buffers)
{
    boost::shared_ptr<std::streambuf>   buffer1(new test_buffer);
    test_buffer                         out1;
    test_buffer                         buffer2;
    std::ostream                        out2(&buffer2);

    logging::add_message(logging::fatal, "hallo");
    CHECK(dynamic_cast<test_buffer&>(*buffer1).no_output());
    CHECK(out1.no_output());
    CHECK(buffer2.no_output());

    logging::add_output(buffer1);
    logging::add_output(out1);
    logging::add_output(out2);

    LOG_FATAL("Hallo Welt");
    CHECK(dynamic_cast<test_buffer&>(*buffer1).test_output("Hallo Welt"));
    CHECK(out1.test_output("Hallo Welt"));
    CHECK(buffer2.test_output("Hallo Welt"));

    logging::remove_output(buffer1);
    logging::remove_output(out1);
    logging::remove_output(out2);
    CHECK(dynamic_cast<test_buffer&>(*buffer1).no_output());
    CHECK(out1.no_output());
    CHECK(buffer2.no_output());
}

TEST(test_log_macros)
{
    test_buffer out;
    logging::add_output(out);
    logging::set_level(logging::fatal);

    LOG_FATAL("foo");
    CHECK(out.test_output("foo"));

    LOG_ERROR("foo");
    CHECK(out.no_output());
    logging::set_level(logging::error);
    LOG_ERROR("foo");
    CHECK(out.test_output("foo"));

    LOG_WARNING("foo");
    CHECK(out.no_output());
    logging::set_level(logging::warning);
    LOG_WARNING("foo");
    CHECK(out.test_output("foo"));

    LOG_INFO("foo");
    CHECK(out.no_output());
    logging::set_level(logging::info);
    LOG_INFO("foo");
    CHECK(out.test_output("foo"));

    LOG_MAIN("foo");
    CHECK(out.no_output());
    logging::set_level(logging::main);
    LOG_MAIN("foo");
    CHECK(out.test_output("foo"));

    LOG_DETAIL("foo");
    CHECK(out.no_output());
    logging::set_level(logging::detail);
    LOG_DETAIL("foo");
    CHECK(out.test_output("foo"));

    LOG_DEBUG("foo");
    CHECK(out.no_output());
    logging::set_level(logging::debug);
    LOG_DEBUG("foo");
#ifdef NDEBUG
    CHECK(out.no_output());
#else
    CHECK(out.test_output("foo"));
#endif

    LOG_ALL("foo");
    CHECK(out.no_output());
    logging::set_level(logging::all);
    LOG_ALL("foo");
#ifdef NDEBUG
    CHECK(out.no_output());
#else
    CHECK(out.test_output("foo"));
#endif

    logging::remove_output(out);
}
