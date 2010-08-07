// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/test_tools.h"
#include "tools/hexdump.h"
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <limits>
#include <iterator>
#include <sstream>

#ifdef max
#   undef max
#endif

#ifdef min
#   undef min
#endif

std::size_t server::test::run(boost::asio::io_service& s)
{
    std::size_t sum = 0;
    std::size_t bridge_zero_loops = 2;

    // for unknown reasons, the run() function does not execute any element ins some occasions, even when one element was posted
    for ( ; bridge_zero_loops != 0; --bridge_zero_loops )
    {
        std::size_t now = s.run();

        if ( now != 0 )
            bridge_zero_loops = 2;

        sum += now;
    }

    return sum;
}

namespace {
    void run_pool_thread(boost::asio::io_service& q, std::size_t& result, boost::mutex& m )
    {
        const std::size_t c = server::test::run(q);

        boost::mutex::scoped_lock lock(m);
        result += c;
    }
}

std::size_t server::test::run(boost::asio::io_service& s, unsigned pool_size)
{
    std::size_t         result = 0;
    boost::mutex        mutex;
    boost::thread_group group;

    for ( ; pool_size; --pool_size )
        group.create_thread(boost::bind(&run_pool_thread, boost::ref(s), boost::ref(result), boost::ref(mutex)));

    group.join_all();

    return result;
}

std::vector<char> server::test::random_body(boost::minstd_rand& random, std::size_t size)
{
    typedef boost::uniform_int<char> distribution_type;
    typedef boost::variate_generator<boost::minstd_rand&, distribution_type> gen_type;

    distribution_type distribution(std::numeric_limits<char>::min(), std::numeric_limits<char>::max());
    gen_type die_gen(random, distribution);

    std::vector<char>   result;
    std::generate_n(std::back_inserter(result), size, die_gen);

    return result;
}

namespace {
    template <class C>
    void add(C& c, const std::string& s)
    {
        c.insert(c.end(), s.begin(), s.end());
    }
}

std::vector<char> server::test::random_chunk(boost::minstd_rand& random, const std::vector<char>& original, std::size_t max_chunk_size)
{
    typedef boost::uniform_int<std::size_t> distribution_type;
    typedef boost::variate_generator<boost::minstd_rand&, distribution_type> gen_type;

    distribution_type distribution(1, max_chunk_size);
    gen_type die_gen(random, distribution);

    std::vector<char>   result;
    const std::string   crlf("\r\n");

    for ( std::vector<char>::const_iterator read_pos = original.begin(); read_pos != original.end(); )
    {
        const std::size_t size = std::min(std::size_t(original.end() - read_pos), die_gen());

        std::ostringstream chunk_header;
        chunk_header << std::hex << size  << "\r\n";

        add(result, chunk_header.str());
        result.insert(result.end(), read_pos, read_pos +size);
        add(result, crlf);

        read_pos += size;
    }

    add(result, "0\r\n\r\n");

    return result;
}

namespace {
    void print_buffer_part(const std::vector<char>& buffer, std::vector<char>::const_iterator error_pos, std::ostream& out)
    {
        std::size_t start_pos = error_pos - buffer.begin();
        start_pos -= std::min(start_pos, 32u);
        start_pos -= start_pos % 16;

        std::size_t size = std::min(64u, buffer.size() - start_pos);

        out << "offset: " << std::hex << start_pos << '\n';
        tools::hex_dump(out, buffer.begin() + start_pos, buffer.begin() + start_pos + size);
    }
}
bool server::test::compare_buffers(const std::vector<char>& org, const std::vector<char>& comp, std::ostream& report)
{
    std::vector<char>::const_iterator org_pos = org.begin();
    std::vector<char>::const_iterator comp_pos = comp.begin();

    for ( ; org_pos != org.end() && comp_pos != comp.end() && *org_pos == *comp_pos; ++org_pos, ++comp_pos)
        ;

    if ( comp_pos != comp.end() || org_pos != org.end() )
    {
        report << "difference found at " << std::hex << (org_pos - org.begin()) << ":\nfirst buffer:\n";
        print_buffer_part(org, org_pos, report);
        report << "\nsecond buffer:\n";
        print_buffer_part(comp, comp_pos, report);

        return false;
    }

    return true;
}

void server::test::wait(const boost::posix_time::time_duration& period)
{
    boost::asio::io_service     queue;
    boost::asio::deadline_timer timer(queue, period);

    timer.wait();
}

