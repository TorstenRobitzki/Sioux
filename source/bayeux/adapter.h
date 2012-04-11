// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_BAYEUX_ADAPTER_H
#define SIOUX_BAYEUX_ADAPTER_H

#include <string>

namespace json {
    class value;
    class string;
}

namespace pubsub {
    class root;
}

namespace bayeux
{
    /**
     * @brief this is interface is used to adapt a user application to a bayeux connector
     *
     * The interface defines 2 hooks directly corresponding to writing functions of the bayeux protocol. Every hook
     * returns an error indicator and a string storing an error message.
     *
     * Service channels aren't supported jet.
     */
    template < class SessionData >
    class adapter
    {
    public:
        /**
         * @brief hook for a new client handshaking with the server
         *
         * @return the first member is used to indicate success (true). If the handshake fails, the function
         *         should return false in the first member and optional an error message in the second member of
         *         the returned pair.
         */
        virtual std::pair< bool, std::string > handshake( const json::value& ext, SessionData& client ) = 0;

        /**
         * @brief hook for a new published message
         *
         * @return the first member is used to indicate success (true). If the publication fails, the function
         *         should return false in the first member and optional an error message in the second member of
         *         the returned pair.
         */
        virtual std::pair< bool, std::string > publish( const json::string& channel, const json::value& data,
            SessionData& client, pubsub::root& root ) = 0;

        virtual ~adapter() {}
    };

} // namespace bayeux

#endif /* SIOUX_BAYEUX_ADAPTER_H */
