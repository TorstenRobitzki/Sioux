// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_RESPONSE_H
#define SIOUX_SOURCE_SERVER_RESPONSE_H

#include "http/http.h"
#include <cassert>

namespace server
{
    class async_response;

    /**
     * @brief small helper to report an error to the connection as last resort
     *
     * This scope guard takes a reference to 
     */
    template <class Connection>
    class report_error_guard
    {
    public:
        report_error_guard(Connection& con, async_response& resp) 
            : con_(&con)
            , response_(&resp) 
            , error_code_(http::http_internal_server_error)
        {}

        report_error_guard(Connection& con, async_response& resp, http::http_error_code ec)
            : con_(&con)
            , response_(&resp) 
            , error_code_(ec)
        {}

        ~report_error_guard()
        {
            if ( con_ )
            {
                assert(response_);
                con_->response_not_possible(*response_, error_code_);
            }
        }

        void dismiss()
        {
            con_ = 0;
        }

    private:
        report_error_guard(const report_error_guard&);
        report_error_guard& operator=(const report_error_guard&);

        Connection*             con_;
        async_response*         response_;
        http::http_error_code   error_code_;
    };

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

