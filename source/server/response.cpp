// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/response.h"

namespace server {

//////////////////
// class async_response
async_response::async_response(const boost::shared_ptr<response_chain_link>& next)
 : next_(next)
 , prev_()
 , pending_buffer_()
 , pending_(false)
{
    assert(next.get());
}

bool async_response::start()
{
    const state s = start_io();

    return s == io_pending || s == buffer_full;
}

void async_response::start_send_data(state current_state, const char* data, std::size_t size)
{
    if ( current_state == buffer_full || current_state == io_pending || ( current_state == done && size != 0 ) )
    {
        next_->issue_async_write(boost::asio::buffer(data, size), boost::enable_shared_from_this<response_chain_link>::shared_from_this());
        next_.reset();
    }
    else
    {
        handle_pending();
    }
}

void async_response::handle_pending()
{
    if ( pending_ )
    {
        assert(prev_.get());
        next_->issue_async_write(pending_buffer_, prev_);
        pending_ = false;
    }

    done_ = true;
    next_.reset();
    prev_.reset();
}

void async_response::issue_async_write(const boost::asio::const_buffer& buffer, const boost::shared_ptr<response_chain_link>& prev)
{
    if ( !done_ )
    {
        pending_        = true;
        pending_buffer_ = buffer;
        prev_           = prev;
    }
    else
    {
        next_->issue_async_write(buffer, prev);
    }
}

void async_response::handle_async_write(const boost::system::error_code& ec, std::size_t bytes_transferred, const boost::shared_ptr<response_chain_link>& me)
{
    next_ = me;   

    const state s = end_send_data(ec, bytes_transferred);

    if ( s == done || s == error )
        handle_pending();
}

void async_response::next_response_waiting()
{
}



} // namespace server 


