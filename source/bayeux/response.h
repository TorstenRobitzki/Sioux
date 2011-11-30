// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_BAYEUX_BAYEUX_RESPONSE_H_
#define SIOUX_BAYEUX_BAYEUX_RESPONSE_H_

#include <boost/asio/buffer.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/system/error_code.hpp>

#include "bayeux/session.h"
#include "http/request.h"
#include "json/json.h"
#include "server/response.h"
#include "server/timeout.h"
#include "server/session_generator.h"

namespace http
{
	class request_header;
}

namespace pubsub
{
	class node_name;
}

namespace bayeux
{
	class connector;
	class session;

	/*!
	 * @brief base class for response, with functions that do not depend on the Connection type
	 */
	class response_base : response_interface
	{
	protected:

		explicit response_base( connector& con );

		/**
		 * @brief dispatcher for the different channels
		 */
		void handle_request( const json::object& request, const std::string& connection_name );
		void handle_handshake( const json::object& request, const std::string& connection_name );
		void handle_connect( const json::object& request, const std::string& connection_name );
		void handle_subscribe( const json::object& request, const std::string& connection_name );

		std::vector< boost::asio::const_buffer > build_response( const json::array& );

		// the response of the bayeux protocol layer
		json::array							bayeux_response_;
		// a buffer for free texts for the http response
		std::string							response_buffer_;

	private:
		// response_interface implementation
		void second_connection_detected();
        void new_messages( const json::array& );

        static std::string extract_channel( const json::object& request );
		static json::string extract_client_id( const json::object& request );
		static pubsub::node_name extract_node_name( const json::object& request );
		static json::object add_session_id( const json::object& response, const json::string& session_id );
		static json::object add_session_id( const json::object& response, const session& session_id );

		connector&							connector_;
	};

	template < class Connection >
	class response :
		private response_base,
		public server::async_response,
		public boost::enable_shared_from_this< response< Connection > >,
	    private boost::noncopyable
	{
	public:
		response( const boost::shared_ptr< Connection >& connection, const http::request_header& request,
				connector& root );

	private:
        virtual void implement_hurry();
        virtual void start();

		void body_read_handler(
			const boost::system::error_code& error,
			const char* buffer,
			std::size_t bytes_read_and_decoded );

		void handle_requests( const json::value& );

		void response_written(
			const boost::system::error_code& error,
			std::size_t bytes_transferred );

		boost::shared_ptr< Connection >				connection_;
		bool										parsed_;
        json::parser								message_parser_;
		// a concatenated list of snippets that form the http response
        std::vector< boost::asio::const_buffer > 	response_;
	};

	// implementation
	template < class Connection >
	response< Connection >::response( const boost::shared_ptr< Connection >& connection, const http::request_header&,
			connector& root )
	  : response_base( root )
	  , connection_( connection )
	  , parsed_( false )
	  , message_parser_()
	{
	}

	template < class Connection >
    void response< Connection >::implement_hurry()
	{
		// the body must have been already read, or otherwise no other request could have been read
	}

	template < class Connection >
    void response< Connection >::start()
	{
		connection_->async_read_body( boost::bind( &response::body_read_handler, this->shared_from_this(), _1, _2, _3
				) );
	}

	template < class Connection >
	void response< Connection >::body_read_handler(
		const boost::system::error_code& error,
		const char* buffer,
		std::size_t bytes_read_and_decoded )
	{
		server::report_error_guard< Connection > guard( *connection_, *this );

		if ( !error )
		{
			if ( bytes_read_and_decoded == 0 && parsed_ )
			{
				message_parser_.flush();

				handle_requests( message_parser_.result() );
				guard.dismiss();
			}
			else if ( bytes_read_and_decoded != 0 && !parsed_ )
			{
				parsed_ = message_parser_.parse( buffer, buffer + bytes_read_and_decoded );
				guard.dismiss();
			}
		}
	}

	template < class Connection >
	void response< Connection >::handle_requests( const json::value& request_container )
	{
		struct visitor_t : json::default_visitor
		{
	        void visit( const json::object& o )
	        {
	        	response_.handle_request( o, connection_ );
	        	handled_ = true;
	        }

	        void visit( const json::array& list )
	        {
	        	list.for_each( *this );
	        }

	        visitor_t( response< Connection >& r, const std::string& connection_name )
	        	: handled_( false )
	        	, response_( r )
	        	, connection_( connection_name )
	        {
	        }

	        bool 					handled_;
			response< Connection >& response_;
			const std::string		connection_;
		} visitor( *this,
			server::socket_end_point_trait< typename Connection::socket_t >::to_text( connection_->socket() ) );

		request_container.visit( visitor );

		if ( !bayeux_response_.empty() )
		{
			response_ = build_response( bayeux_response_ );
			connection_->async_write(
				response_,
				boost::bind( &response::response_written, this->shared_from_this(), _1, _2 ),
				*this );
		}
	}

	template < class Connection >
	void response< Connection >::response_written(
		const boost::system::error_code&, std::size_t )
	{
		connection_->response_completed( *this );
	}


}

#endif /* SIOUX_BAYEUX_BAYEUX_RESPONSE_H_ */
