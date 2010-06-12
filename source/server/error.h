// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SOURCE_SIOUX_SERVER_ERROR_H
#define SOURCE_SIOUX_SERVER_ERROR_H

#include "http/http.h"
#include "server/response.h"
#include <boost/enable_shared_from_this.hpp>
#include <boost/utility.hpp>

namespace server {

    template <class Connection>
    class error_response : public async_response, 
                           public boost::enable_shared_from_this<proxy_response<Connection> >, 
                           private boost::noncopyable
    {
    public:
        explicit error_response(http::http_error_code ec);
    };
} // namespace server


