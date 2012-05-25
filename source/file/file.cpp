// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "file/file.h"
#include "tools/asstring.h"

namespace file
{
    file_root::file_root( const boost::filesystem::path& root_file_name )
        : root_( boost::filesystem::canonical( root_file_name ) )
    {
        if ( !exists( root_ ) )
            throw std::runtime_error( "file_root: " + tools::as_string( root_ ) + " doesn't exists!" );

        if ( !is_directory( root_ ) )
            throw std::runtime_error( "file_root: " + tools::as_string( root_ ) + " is not a directory!" );
    }

    boost::filesystem::path file_root::check_canonical( const http::request_header& header ) const
    {
        const boost::filesystem::path requested_file =
            boost::filesystem::canonical( root_ / tools::as_string( header.uri() ) );

        if ( requested_file.generic_string().find( root_.generic_string() ) == 0 )
            return requested_file;

        return boost::filesystem::path();
    }

}
