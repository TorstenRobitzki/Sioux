#include "json_handler/connector.h"
#include "tools/substring.h"

namespace json
{
namespace detail
{
    /**
     * @brief maps an url and a method to a handler
     */
    class handler_map
    {
    public:
        void add_handler( const std::vector< std::string >& urls, const std::vector< http::http_method_code >& methods, const connector::handler_t& );

        const connector::handler_t* find_handler( const tools::substring& url, http::http_method_code method );
        const connector::handler_t* find_handler( const std::string& url, http::http_method_code method );
    private:

        connector::handler_t handler_;
    };
}
}
