#include "http/test_tools.h"
#include "tools/hexdump.h"
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <limits>
#include <iterator>
#include <sstream>
#include <algorithm>

#ifdef max
#   undef max
#endif 

#ifdef min
#   undef min
#endif

std::vector<char> http::test::random_body(boost::minstd_rand& random, std::size_t size)
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

std::vector<char> http::test::random_chunk(boost::minstd_rand& random, const std::vector<char>& original, std::size_t max_chunk_size)
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
        start_pos -= std::min(start_pos, std::size_t(32u));
        start_pos -= start_pos % 16;

        std::size_t size = std::min(std::size_t(64u), static_cast<std::size_t>(buffer.size() - start_pos));

        out << "offset: " << std::hex << start_pos << '\n';
        tools::hex_dump(out, buffer.begin() + start_pos, buffer.begin() + start_pos + size);
    }
}

bool http::test::compare_buffers(const std::vector<char>& org, const std::vector<char>& comp, std::ostream& report)
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


