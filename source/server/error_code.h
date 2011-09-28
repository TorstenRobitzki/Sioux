// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SOURCE_SIOUX_ERROR_CODE_H
#define SOURCE_SIOUX_ERROR_CODE_H

#include <boost/system/error_code.hpp>

namespace server {

    enum error_codes 
    {
    	no_error,
        canceled_by_error,
        limit_reached,
        time_out
    };

    class connection_error_category : public boost::system::error_category 
    {
        virtual const char *     name() const;
        virtual std::string      message( int ev ) const;
    };

    boost::system::error_code make_error_code(error_codes e);

} // server

#endif // include guard

