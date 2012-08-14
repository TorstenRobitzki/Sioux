// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_FILE_FILE_H_
#define SIOUX_FILE_FILE_H_

#include "file/response.h"
#include "http/request.h"
#include "server/error.h"
#include "server/connection.h"

#include <boost/filesystem.hpp>

namespace file
{
    /**
     * @brief defines a root for static file delivery
     *
     * If the requested file is a directory, a file named 'index.html' or 'index.htm' will be delivered
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

    private:
        boost::filesystem::path check_canonical( const http::request_header& header ) const;

        const boost::filesystem::path   root_;
    };

    /**
     * @brief adds a static file handler to the given server
     * @relates file_root
     *
     * @param server a server implementation with a add_action function
     * @param filter the URI start used as a filter
     * @param root_file_name the directory in the filesystem to read the file from
     */
    template < class Server >
    void add_file_handler( Server& server, const char* filter, const boost::filesystem::path& root_file_name );

    // implementation
    template < class Connection >
    boost::shared_ptr< server::async_response > file_root::create_response(
        const boost::shared_ptr< Connection >&                    connection,
        const boost::shared_ptr< const http::request_header >&    header )
    {
        try
        {
            boost::filesystem::path file_name = check_canonical( *header );

            if ( file_name.empty() )
                return boost::shared_ptr< server::async_response >(
                    new server::defered_error_response< Connection >( connection, http::http_forbidden ) );

            if ( is_directory( file_name ) )
            {
                boost::filesystem::path changed_file_name = file_name / "index.html";

                if ( exists( changed_file_name ) && !is_directory( changed_file_name ) )
                {
                    file_name = changed_file_name;
                }
                else
                {
                    boost::filesystem::path changed_file_name = file_name / "index.htm";

                    if ( exists( changed_file_name ) && !is_directory( changed_file_name ) )
                    {
                        file_name = changed_file_name;
                    }
                    else
                    {
                        return boost::shared_ptr< server::async_response >(
                            new server::defered_error_response< Connection >( connection, http::http_not_found ) );
                    }
                }
            }
        
            return boost::shared_ptr< server::async_response >(
                new response< Connection >( connection, file_name ) );
        }
        catch ( ... )
        {
            return boost::shared_ptr< server::async_response >(
                new server::defered_error_response< Connection >( connection, http::http_not_found ) );
        }
    }

    template < class Server >
    void add_file_handler( Server& server, const char* filter, const boost::filesystem::path& root_file_name )
    {
        typedef server::connection< typename Server::trait_t > connection_t;
        server.add_action( filter, boost::bind( &file_root::create_response< connection_t >, file_root( root_file_name ), _1, _2 ) );
    }

}

#endif /* SIOUX_FILE_FILE_H_ */
