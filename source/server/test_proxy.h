// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/proxy.h"
#include "server/test_socket.h"
#include <boost/utility.hpp>

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
        explicit proxy_config(const std::string& simulate_response);

        /**
         * @brief constructs a proxy_config that will return the passed response text, if asked for a connection
         */
        explicit proxy_config(const socket<const char*>& socket);

        ~proxy_config();

        /**
         * @brief data that was received by the simulated orgin server.
         */
        std::string received() const;

        /**
         * @brief simulate IO from or to the orgin servers
         * @brief returns false, if no IO was done
         */
        bool process();

    private:

        virtual void async_get_proxy_connection(
            const tools::dynamic_type&          connection_type,
            const std::string&                  orgin,
            std::auto_ptr<connect_callback>&    call_back);

        virtual void release_connection(
            const tools::dynamic_type&          connection_type,
            void*                               connection);

        const std::vector<char>                 simulate_response_;
        socket<const char*>                     socket_;
        bool                                    socket_in_use_;

        std::auto_ptr<connect_callback>         call_back_;
    };

} // namespace test
} // namespace server

