#ifndef SIOUX_SOURCE_ASIO_MOCKS_JSON_MSG_H
#define SIOUX_SOURCE_ASIO_MOCKS_JSON_MSG_H

#include <string>
#include "asio_mocks/test_io_plan.h"

namespace json {
    class value;
}


namespace asio_mocks
{

    /**
     * @brief constructs a application/json message out of the given text
     *
     * Within the given text, single quote marks (') are replaced by double quote marks (").
     *
     * @relates asio_mocks::read
     */
    read json_msg( const std::string& txt );

    /**
     * @copydoc asio_mocks::json_msg(const std::string&)
     * @relates asio_mocks::read
     */
    template < std::size_t S >
    read json_msg( const char(&txt)[S] )
    {
        return json_msg( std::string( txt ) );
    }

    /**
     * @brief constructs a application/json message out of the given json value
     * @relates asio_mocks::read
     */
    read json_msg( const json::value& payload );
}

#endif

