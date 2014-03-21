#include <utility>
#include <string>
#include "tools/substring.h"

namespace server
{
    /**
     * @brief checks whether a given uri fits to an other
     */
    class fitting_uri
    {
    public:
        /**
         * @brief constructs a fitting_uri that compares the given uri with a set of
         *        filters given by the operator()
         */
        explicit fitting_uri( const tools::substring& uri );

        bool operator()( const std::string& filter ) const;

        template < class P >
        bool operator()( const std::pair< std::string, P >& pair ) const
        {
            return (*this)( pair.first );
        }

    private:
        static tools::substring uri_without_trailing_slash( tools::substring::const_iterator, tools::substring::const_iterator );
        const tools::substring  uri_;
    };
}
