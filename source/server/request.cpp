// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/request.h"
#include <algorithm>
#include <cassert>

namespace server {

    //////////////////////////
    // class request_header
    request_header::request_header()
        : write_ptr_(0)
        , parse_ptr_(0)
        , error_(parsing)
    {
    }

    request_header::request_header(const request_header& old_header, copy_trailing_buffer_t)
        : write_ptr_(old_header.write_ptr_ - old_header.parse_ptr_)
        , parse_ptr_(0)
        , error_(parsing)
    {
        std::copy(&old_header.buffer_[old_header.parse_ptr_], &old_header.buffer_[old_header.write_ptr_], &buffer_[0]);
    }
    
    std::pair<char*, std::size_t> request_header::read_buffer()
    {
        assert(write_ptr_ < sizeof buffer_);
        return std::make_pair(&buffer_[write_ptr_], sizeof buffer_ - write_ptr_);
    }
    
    bool request_header::parse(std::size_t& size)
    {
        write_ptr_ += size;
        size        = 0;

        assert(write_ptr_ < sizeof buffer_);

        if ( write_ptr_ == sizeof buffer_ )
        {
            error_ = buffer_full;
            return true;
        }

        // TODO
        if ( std::string(&buffer_[0], &buffer_[write_ptr_]).find("\r\n\r\n") != std::string::npos )
        {
            error_ = ok;
            return true;
        }

        return false;
    }
    
    request_header::error_code request_header::state() const
    {
        return error_;
    }
    

} // namespace server

