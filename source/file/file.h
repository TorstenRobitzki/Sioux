// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_FILE_FILE_H_
#define SIOUX_FILE_FILE_H_

#include "file/response.h"
#include "http/request.h"
#include "server/error.h"

#include <boost/filesystem.hpp>

namespace file
{
    /**
     * @brief defines a root for static file delivery
     */
    class file_root
    {
    public:
        /**
         * @brief constructs a file_root with the base directory
         */
        explicit file_root( const boost::filesystem::path& root_file_name );

        /**
         * @brief creates a response object that will deliver the requested static content
         *        or an error if it was not possible to deliver.
         */
        template < class Connection >
        boost::shared_ptr< server::async_response > create_response(
            const boost::shared_ptr< Connection >&                    connection,
            const boost::shared_ptr< const http::request_header >&    header );

        boost::filesystem::path check_canonical( const http::request_header& header ) const;
    private:
        const boost::filesystem::path   root_;
    };

    // implementation
    template < class Connection >
    boost::shared_ptr< server::async_response > file_root::create_response(
        const boost::shared_ptr< Connection >&                    connection,
        const boost::shared_ptr< const http::request_header >&    header )
    {
        const boost::filesystem::path file_name = check_canonical( *header );

        if ( file_name.empty() )
            return boost::shared_ptr< server::async_response >(
                new server::defered_error_response< Connection >( connection, http::http_forbidden ) );

        return boost::shared_ptr< server::async_response >(
            new response< Connection >( connection, file_name ) );
    }

}

#endif /* SIOUX_FILE_FILE_H_ */
