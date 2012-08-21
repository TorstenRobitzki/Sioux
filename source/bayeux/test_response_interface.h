// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef TEST_RESPONSE_INTERFACE_H_
#define TEST_RESPONSE_INTERFACE_H_

#include "bayeux/session.h"
#include "json/json.h"

#include <vector>

namespace bayeux
{
namespace test
{
    /**
     *  @brief implementation of the bayeux::response_interface for testing
     */
    class response_interface : public bayeux::response_interface
    {
    public:
        response_interface();

        /**
         * @brief returns the number of times, the second_connection_detected() function was called
         */
        int number_of_second_connection_detected() const;

        /**
         * @brief returns the message array that was passed to the messages() function, the last time, that function
         *        was called.
         */
        json::array new_message() const;

        /**
         * @brief the recorded value that where passed to messages()
         */
        const std::vector< json::array >& messages() const;

    private:
        // response_interface-implementatin
        void second_connection_detected();
        void messages( const json::array& message, const json::string& );

        int                         second_connections_detected_;
        std::vector< json::array >  messages_;
    };

}
}
#endif /* TEST_RESPONSE_INTERFACE_H_ */
