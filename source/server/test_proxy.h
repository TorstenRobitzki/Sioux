// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/proxy.h"
#include "server/test_socket.h"
#include <boost/utility.hpp>
#include <boost/asio/io_service.hpp>

namespace server
{
namespace test {

    /**
     * @brief configuration that provides server::test::socket<const char*> connections that 
     *        simulate the IO with an orgin server
     */
    class proxy_config : public proxy_config_base, boost::noncopyable
    {
    public:
        /**
         * @brief constructs a proxy_config that will send the passed response text
         */
        explicit proxy_config(boost::asio::io_service& queue, const std::string& simulate_response);

        enum error_type {
            /** no error at all */
            no_error,
            /** async_get_proxy_connection() will throw an exception */
            connection_not_possible,
            /** async_get_proxy_connection() will call the passed callback, but with an error */
            error_while_connecting
        };

        /**
         * @brief constructs a proxy, that simulates the passes error situation
         */
        explicit proxy_config(boost::asio::io_service& queue, error_type error);

        /**
         * @brief constructs a proxy_config that will return the passed response text, if asked for a connection
         */
        explicit proxy_config(socket<const char*>& socket);

        ~proxy_config();

        /**
         * @brief data that was received by the simulated orgin server.
         */
        std::string received() const;

        /**
         * @brief returns the host and port that was last connected by a request
         */
        std::pair<std::string, unsigned> connected_orgin_server() const;

        boost::asio::io_service& get_io_service();
    private:

        void call_cb(connect_callback* p);

        virtual void async_get_proxy_connection(
            const tools::dynamic_type&          connection_type,
            const tools::substring&             orgin_host,
            unsigned                            orgin_port,
            std::auto_ptr<connect_callback>&    call_back);

        virtual void release_connection(
            const tools::dynamic_type&          connection_type,
            void*                               connection);

        boost::asio::io_service&                io_service_;
        const std::vector<char>                 simulate_response_;
        const error_type                        error_type_;
        socket<const char*>                     socket_;
        bool                                    socket_in_use_;
        std::pair<std::string, unsigned>        requested_orgin_;
    };

} // namespace test
} // namespace server

