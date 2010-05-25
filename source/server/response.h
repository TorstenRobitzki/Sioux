// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_RESPONSE_H
#define SIOUX_SOURCE_SERVER_RESPONSE_H

namespace server
{
    /**
     * @brief base class for asynchronous responses to an http request
     */
    class async_response
    {
    public:
        /**
         * @brief default constructor
         */
        async_response();

        /**
         * @brief function to indicate that responses to later requests are now ready to send data
         *
         * This function calls implement_hurry() once. Every subsequent calls to hurry() will have no
         * effect. This function is intendet to unblock long polling http connections
         */
        void hurry();

        /**
         * @brief returns true, if hurry() was called at least once
         */
        bool asked_to_hurry() const;

        virtual ~async_response() {}

        /**
         * @brief default implementation does nothing
         */
        virtual void implement_hurry();
    private:
        bool hurryed_;
    };
} // namespace server

#endif

