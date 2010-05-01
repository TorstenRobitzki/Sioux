// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SOURCE_SIOUX_CONNECTION_H
#define SOURCE_SIOUX_CONNECTION_H

#include "server/request.h"
#include <boost/asio/error.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <cassert>

// delete me if you see me
#include <iostream>

namespace server 
{
	/**
	 * @brief representation of physical connection from a client to this server
	 *
	 * It's the responsibility of the connection object to parse incomming requests 
	 * and order outgoing responses.
	 */
	template <class Trait, class Connection>
	class connection : public boost::enable_shared_from_this<connection<Trait, Connection> >
	{
	public:
		connection(const Connection& con, const Trait& trait);

        /**
         * @brief starts the first asynchron read on the connection passed to the c'tor
         */
        void start();
	private:
        void issue_header_read();
		void handle_read(const boost::system::error_code& e, std::size_t bytes_transferred);

		void handle_request_header(const boost::shared_ptr<const request_header>& new_request);

        Connection                          connection_;
        Trait                               trait_;

        boost::shared_ptr<request_header>   current_request_;
 	};

    /**
     * @brief creates a new connection object and calls start() on it.
     * @relates connection
     */
	template <class Trait, class Connection>
    boost::shared_ptr<connection<Trait, Connection> > create_connection(const Connection& con, const Trait& trait);

	template <class Trait, class Connection>
	connection<Trait, Connection>::connection(const Connection& con, const Trait& trait)
        : connection_(con)
        , trait_(trait)
        , current_request_()
    {
    }

	template <class Trait, class Connection>
    void connection<Trait, Connection>::start()
    {
        current_request_.reset(new request_header);
        issue_header_read();
    }

	template <class Trait, class Connection>
    void connection<Trait, Connection>::issue_header_read()
    {
        const std::pair<char*, std::size_t> buffer = current_request_->read_buffer();
        assert(buffer.first && buffer.second);

        connection_.async_read_some(
            boost::asio::buffer(buffer.first, buffer.second),
            boost::bind(&connection::handle_read, shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred)
        );
    }

	template <class Trait, class Connection>
	void connection<Trait, Connection>::handle_read(const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        if (!error)
        {
            while ( bytes_transferred != 0 && current_request_->parse(bytes_transferred) )
            {
                handle_request_header(current_request_);
                current_request_.reset(new request_header(*current_request_, request_header::copy_trailing_buffer));
            }

            issue_header_read();
        }
    }

	template <class Trait, class Connection>
    void connection<Trait, Connection>::handle_request_header(const boost::shared_ptr<const request_header>& new_request)
    {
        trait_.create_response(new_request);
    }

	template <class Trait, class Connection>
    boost::shared_ptr<connection<Trait, Connection> > create_connection(const Connection& con, const Trait& trait)
    {
        const boost::shared_ptr<connection<Trait, Connection> > result(new connection<Trait, Connection>(con, trait));
        result->start();

        return result;
    }



} // namespace server

#endif

