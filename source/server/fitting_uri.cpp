#include "server/fitting_uri.h"

namespace server {

    fitting_uri::fitting_uri( const tools::substring& s )
        : uri_( uri_without_trailing_slash( s.begin(), s.end() ) )
    {
    }

    bool fitting_uri::operator()( const std::string& filter_input ) const
    {
        const tools::substring filter = uri_without_trailing_slash( filter_input.data(), filter_input.data() + filter_input.size() );

        if ( uri_.size() < filter.size() )
            return false;

        if ( uri_.size() == filter.size() )
            return uri_ == filter;

        assert( uri_.size() > filter.size() ); 
        // now, the filter is shorter than the uri. Make sure, that the filter with a slash (/) would compare with the uri by looking up a slash in the uri
        if ( *( uri_.begin() + filter.size() ) != '/' )
            return false;

        const tools::substring to_be_compared_with( uri_.begin(), uri_.begin() + filter.size() );

        return filter == to_be_compared_with;
    }

    tools::substring fitting_uri::uri_without_trailing_slash(
        tools::substring::const_iterator begin, tools::substring::const_iterator end )
    {
        return begin == end || *( end - 1 ) != '/'
            ? tools::substring( begin, end )
            : tools::substring( begin, end - 1 );
    }

}

