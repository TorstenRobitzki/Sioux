#include "json_handler/handler_map.h"

namespace json
{
namespace detail
{
    void handler_map::add_handler( const std::vector< std::string >&, const std::vector< http::http_method_code >&, const connector::handler_t& new_handler )
    {
        handler_ = new_handler;
    }

    const connector::handler_t* handler_map::find_handler( const tools::substring& url, http::http_method_code method )
    {
        return handler_.empty()
            ? 0
            : &handler_;
    }

    const connector::handler_t* handler_map::find_handler( const std::string& url, http::http_method_code method )
    {
        return find_handler( tools::substring( url.data(), url.data() + url.size() ), method );
    }

}
}
