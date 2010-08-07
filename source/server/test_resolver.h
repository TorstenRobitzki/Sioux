// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_TEST_RESOLVER_H
#define SIOUX_SOURCE_SERVER_TEST_RESOLVER_H

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/basic_resolver.hpp>
#include <boost/asio/error.hpp>
#include <boost/bind.hpp>

namespace server {
namespace test {

    /**
     * @brief resolver for test purpose
     */
    class resolver
    {
    public:
        typedef boost::asio::ip::basic_resolver_query<boost::asio::ip::tcp> query;
        typedef boost::asio::ip::tcp::resolver_iterator                     iterator;

        explicit resolver(boost::asio::io_service& q) : queue_(q) {}

        /**
         * @brief posts a call to handler to the io_service given to the c'tor
         *
         * If q.host_name() is equal to "invalid", handler will be called with an error.
         */
        template<typename ResolveHandler>
        void async_resolve(
            const query & q,
            ResolveHandler handler);

    private:
        boost::asio::io_service&    queue_;
    };

    // implementation
    template<typename ResolveHandler>
    void resolver::async_resolve(
        const query & q,
        ResolveHandler handler)
    {
        if ( q.host_name() == "invalid" )
        {
            queue_.post(boost::bind<void>(handler, make_error_code(boost::asio::error::host_not_found), iterator()));
        }
        else
        {
            /// @todo
            queue_.post(boost::bind<void>(handler, make_error_code(boost::asio::error::host_not_found), iterator()));
        }
    }

} // namespace test
} // namespace server


#endif // include guard
