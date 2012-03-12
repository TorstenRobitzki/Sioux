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
#include "response.h"

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
    template < class Timer >
	class connector;
	class session;

	/*!
	 * @brief base class for response, with functions that do not depend on the Connection type
	 */
	template < class Timer >
	class response_base : public response_interface
	{
	protected:
		explicit response_base( connector< Timer >& con );

		~response_base();

		/**
		 * @brief dispatcher for the different channels
		 */
		void handle_request( const json::object& request, const boost::shared_ptr< response_interface >& self,
		    const std::string& connection_name );
		void handle_handshake( const json::object& request, const std::string& connection_name );
		void handle_connect( const json::object& request, const boost::shared_ptr< response_interface >& self );
		void handle_subscribe( const json::object& request );

		json::object build_connect_response( const json::object& request, const json::string& session_id );

		std::vector< boost::asio::const_buffer > build_response( const json::array& );

		// the response of the bayeux protocol layer
		json::array							bayeux_response_;
		// a buffer for free texts for the http response
		std::string							response_buffer_;

		// the connect request that broad this response to block on
        json::object                        blocking_connect_;
        session*                            session_;
	private:
        json::string check_client_id( const json::object& request, const json::string& response_channel);
        bool check_session( const json::object& request, const json::string& id, const json::string& reponse_channel );
        json::string check_subscription( const json::object& request, const json::string& response_channel );
        // checks whether the connection type is given and valid.
        bool check_connection_type( const json::object& request, const json::string& session_id );

        void copy_id_field( const json::object& from, json::object& to ) const;
        static json::string extract_channel( const json::object& request );
		static json::string extract_client_id( const json::object& request );
		static pubsub::node_name extract_node_name( const json::object& request );
		static json::object add_session_id( const json::object& response, const json::string& session_id );
		static json::object add_session_id( const json::object& response, const session& session_id );

		connector< Timer >& connector_;
	};

	template < class Connection >
	class response :
		public response_base< typename Connection::trait_t::timeout_timer_type >,
		public server::async_response,
		public boost::enable_shared_from_this< response< Connection > >,
	    private boost::noncopyable
	{
	public:
		response( const boost::shared_ptr< Connection >& connection, const http::request_header& request,
				connector< typename Connection::trait_t::timeout_timer_type >& root );

	private:
        virtual void implement_hurry();
        virtual void start();
		void body_read_handler(
			const boost::system::error_code& error,
			const char* buffer,
			std::size_t bytes_read_and_decoded );

		void handle_requests( const json::value& );

        // response_interface implementation
        void second_connection_detected();
        void messages( const json::array&, const json::string& session_id );

        void write_reponse();

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
			connector< typename Connection::trait_t::timeout_timer_type >& root )
	  : response_base< typename Connection::trait_t::timeout_timer_type >( root )
	  , connection_( connection )
	  , parsed_( false )
	  , message_parser_()
	{
	}

	template < class Connection >
    void response< Connection >::implement_hurry()
	{
	    if ( !this->blocking_connect_.empty() )
	        this->session_->hurry();
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
	    server::close_connection_guard< Connection > guard( *connection_, *this );

		if ( error )
		{
		    connection_->trait().log_error( *connection_, "receiving bayeux request body", error, bytes_read_and_decoded );
		    return;
		}

        try
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
        catch ( const std::exception& e )
        {
            connection_->trait().error_executing_request_handler( *connection_, http::request_header(), e.what() );
        }
	}

	template < class Connection >
	void response< Connection >::handle_requests( const json::value& request_container )
	{
		struct visitor_t : json::default_visitor
		{
	        void visit( const json::object& o )
	        {
	        	response_.handle_request( o, response_.shared_from_this(), connection_ );
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

	        bool 					                        handled_;
			response< Connection >&                         response_;
			const std::string		                        connection_;
		} visitor( *this,
			server::socket_end_point_trait< typename Connection::socket_t >::to_text( connection_->socket() ) );

		request_container.visit( visitor );

        if ( this->blocking_connect_.empty() )
            write_reponse();
	}

    template < class Connection >
    void response< Connection >::second_connection_detected()
    {
        assert( this->session_ );
        messages( json::array(), this->session_->session_id() );
    }

    template < class Connection >
	void response< Connection >::messages( const json::array& msg, const json::string& session_id )
	{
        this->bayeux_response_ += msg;
        this->bayeux_response_.add( build_connect_response( this->blocking_connect_, session_id ) );

        write_reponse();
	}

    template < class Connection >
    void response< Connection >::write_reponse()
    {
        response_ = build_response( this->bayeux_response_ );
        connection_->async_write(
            response_,
            boost::bind( &response::response_written, this->shared_from_this(), _1, _2 ),
            *this );
    }

    template < class Connection >
	void response< Connection >::response_written(
		const boost::system::error_code& ec, std::size_t size )
	{
        if ( !ec )
        {
            connection_->response_completed( *this );
        }
        else
        {
            connection_->response_not_possible( *this );
        }
	}


}

#endif /* SIOUX_BAYEUX_BAYEUX_RESPONSE_H_ */
