// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_RESPONSE_H
#define SIOUX_SOURCE_SERVER_RESPONSE_H

namespace server
{
    class async_response
    {
    public:
        async_response();
        void hurry();

        bool asked_to_hurry() const;
        virtual ~async_response() {}
    private:
        virtual void implement_hurry();
        bool hurryed_;
    };
} // namespace server

#endif

