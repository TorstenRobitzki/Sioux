// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_BAYEUX_BAYEUX_RESPONSE_H_
#define SIOUX_BAYEUX_BAYEUX_RESPONSE_H_

#include <boost/asio/buffer.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>

#include "bayeux/session.h"
#include "http/request.h"
#include "http/header_names.h"
#include "http/parser.h"
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
    template < class Timer >
	class connector;
	class session;

	/*!
	 * @brief base class for response, with functions that do not depend on the Connection type
	 */
	template < class Timer >
	class response_base : public response_interface
	{
	public:
        /**
         * @brief dispatcher for the different channels
         */
        void handle_request( const json::object& request, const boost::shared_ptr< response_interface >& self,
            const std::string& connection_name, bool last_message );
	protected:
		explicit response_base( connector< Timer >& con );

		~response_base();

		void handle_handshake( const json::object& request, const std::string& connection_name );
		void handle_connect( const json::object& request, const boost::shared_ptr< response_interface >& self,
		    bool last_message );
        void handle_disconnect( const json::object& request );
		void handle_subscribe( const json::object& request );
        void handle_unsubscribe( const json::object& request );
        void handle_publish( const json::string& channel, const json::object& request );

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
		response( const boost::shared_ptr< Connection >&                            connection,
		          const boost::shared_ptr< const http::request_header >&            request,
				  connector< typename Connection::trait_t::timeout_timer_type >&    root );

	private:
		typedef typename Connection::trait_t trait_t;

        virtual void implement_hurry();
        virtual void start();
        virtual const char* name() const;
		void body_read_handler(
			const boost::system::error_code& error,
			const char* buffer,
			std::size_t bytes_read_and_decoded );

		void handle_requests( const json::value& );
		void handle_form_requests( const std::string& );

        // response_interface implementation
        void second_connection_detected();
        void messages( const json::array&, const json::string& session_id );

        void write_reponse();

        void response_written(
			const boost::system::error_code& error,
			std::size_t bytes_transferred );

        void connection_time_out( const boost::system::error_code& error );

		boost::shared_ptr< Connection >				            connection_;
		const boost::shared_ptr< const http::request_header >   request_;

		// used when the message is application/json encoded
		bool										            parsed_;
        json::parser								            message_parser_;

        // used when the message is application/x-www-form-urlencoded
        bool                                                    form_encoded_;
        std::vector< char >                                     form_body_;

		// a concatenated list of snippets that form the http response
        std::vector< boost::asio::const_buffer > 	            response_;

        typedef typename Connection::trait_t::timeout_timer_type timer_t;
        timer_t                                                 timer_;
	};


	// implementation
	namespace log
	{
	    struct no_logging {};
	    struct has_logging {};

	    template < class Connection >
	    has_logging enabled( const Connection* c,
	        typename Connection::trait_t::bayeux_logging_enabled* f = 0 )
	    {
	        return has_logging();
	    }

        template < class Connection >
        no_logging enabled( ... )
        {
            return no_logging();
        }

        template < class Connection >
        void bayeux_start_response( Connection& con, const has_logging& )
        {
            con.trait().bayeux_start_response( con );
        }

        template < class Connection, class Payload >
        void bayeux_handle_requests( Connection& con, const Payload& request_container, const has_logging& )
        {
            con.trait().bayeux_handle_requests( con, request_container );
        }

        template < class Connection >
        void bayeux_new_request( Connection& con, const http::request_header& header, const has_logging& )
        {
            con.trait().bayeux_new_request( con, header );
        }

        template < class Connection >
        void bayeux_blocking_connect( Connection& con, const json::object& blocking_request, const has_logging& )
        {
            con.trait().bayeux_blocking_connect( con, blocking_request );
        }

        template < class Connection >
        void bayeux_start_response( Connection&, const no_logging& ) {}

        template < class Connection, class Payload >
        void bayeux_handle_requests( Connection&, const Payload&, const no_logging& ) {}

        template < class Connection >
        void bayeux_new_request( Connection& con, const http::request_header& header, const no_logging& ) {}

        template < class Connection >
        void bayeux_blocking_connect( Connection& con, const json::object& blocking_request, const no_logging& ) {}
	}

	template < class Connection >
	response< Connection >::response(
	    const boost::shared_ptr< Connection >&                          connection,
	    const boost::shared_ptr< const http::request_header >&          header,
	    connector< typename Connection::trait_t::timeout_timer_type >&  root )
	  : response_base< typename Connection::trait_t::timeout_timer_type >( root )
	  , connection_( connection )
	  , request_( header )
	  , parsed_( false )
	  , message_parser_()
	  , form_encoded_( false )
	  , form_body_()
	  , response_()
	  , timer_( root.queue() )
	{
        log::bayeux_new_request( *connection_, *header, log::enabled< Connection >( connection_.get() ) );
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
        log::bayeux_start_response( *connection_, log::enabled< Connection >( connection_.get() ) );

        if ( request_->body_expected() )
        {
            form_encoded_ = request_->option_available( http::content_type_header, http::application_x_www_from_urlencded );

            connection_->async_read_body( boost::bind( &response::body_read_handler, this->shared_from_this(), _1, _2, _3
                    ) );
        }
        else
        {
            tools::substring scheme, authority, path, query, fragment;
            // lets see, if the query contains a bayeux message
            http::split_url( request_->uri(), scheme, authority, path, query, fragment );
            handle_form_requests( http::url_decode( query ) );
        }
	}

    template < class Connection >
    const char* response< Connection >::name() const
    {
        return "bayeux::response";
    }

    template < class Connection >
	void response< Connection >::body_read_handler(
		const boost::system::error_code& error,
		const char* buffer,
		std::size_t bytes_read_and_decoded )
	{
	    server::close_connection_guard< Connection > guard( *connection_, *this );

		if ( error && error != boost::asio::error::eof )
		{
		    connection_->trait().log_error( *connection_, "receiving bayeux request body", error, bytes_read_and_decoded );
		    return;
		}

		if ( form_encoded_ )
		{
		    form_body_.insert( form_body_.end(), buffer, buffer + bytes_read_and_decoded );

		    if ( bytes_read_and_decoded == 0 )
		    {
		        handle_form_requests(
		            http::form_decode( tools::substring( &form_body_[ 0 ], &form_body_[ 0 ] + form_body_.size() ) ) );
		    }

		    guard.dismiss();
		}
		else
		{
            if ( bytes_read_and_decoded == 0 && parsed_ )
            {
                message_parser_.flush();
                handle_requests( message_parser_.result() );
                guard.dismiss();
            }
            else if ( bytes_read_and_decoded != 0 && !parsed_ )
            {
                parsed_ = message_parser_.parse( buffer, buffer + bytes_read_and_decoded ).second;
                guard.dismiss();
            }
            else
            {
                connection_->trait().log_error( *connection_, "unexpected state while reading body:",
                    bytes_read_and_decoded, parsed_ );
            }
		}
	}

	namespace details
	{

	    template < class Connection >
        struct handle_requests_visitor : json::default_visitor
        {
            void visit( const json::object& o )
            {
                response_.handle_request( o, response_.shared_from_this(), connection_, last_ );
                handled_ = true;
            }

            void visit( const json::array& list )
            {
                for ( std::size_t i = 0, size = list.length(); i != size; ++i )
                {
                    last_ = i + 1  == size;
                    list.at( i ).visit( *this );
                }
            }

            handle_requests_visitor( response< Connection >& r, const std::string& connection_name )
                : handled_( false )
                , last_( true )
                , response_( r )
                , connection_( connection_name )
            {
            }

            bool                                            handled_;
            bool                                            last_;
            response< Connection >&                         response_;
            const std::string                               connection_;
        };
	}

	template < class Connection >
	void response< Connection >::handle_requests( const json::value& request_container )
	{
	    log::bayeux_handle_requests( *connection_, request_container, log::enabled< Connection >( connection_.get() ) );

	    details::handle_requests_visitor< Connection > visitor( *this,
			server::socket_end_point_trait< typename Connection::socket_t >::to_text( connection_->socket() ) );

		request_container.visit( visitor );

        if ( this->blocking_connect_.empty() )
        {
            write_reponse();
        }
        else
        {
            assert( this->session_ );

            log::bayeux_blocking_connect( *connection_, this->blocking_connect_, log::enabled< Connection >( connection_.get() ) );
            timer_.expires_from_now( this->session_->long_polling_timeout() );
            timer_.async_wait( boost::bind( &response::connection_time_out, this->shared_from_this(), _1 ) );
        }
	}

    template < class Connection >
    void response< Connection >::handle_form_requests( const std::string& body )
    {
        const std::vector< std::pair< tools::substring, tools::substring > > messages =
            http::split_query( tools::substring( body.c_str(), body.c_str() + body.size() ) );

        unsigned message_cnt = 0;
        for ( std::vector< std::pair< tools::substring, tools::substring > >::const_iterator msg = messages.begin();
            msg != messages.end(); ++msg )
        {
            if ( msg->first == "message" )
            {
                handle_requests( json::parse( msg->second.begin(), msg->second.end() ) );
                ++message_cnt;
            }
        }

        if ( message_cnt == 0 )
            throw std::runtime_error( "form/url encoded bayeux message body without message" );
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
        timer_.cancel();

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

    template < class Connection >
    void response< Connection >::connection_time_out( const boost::system::error_code& error )
    {
        if ( !error )
        {
            this->session_->timeout();
        }
    }


}

#endif /* SIOUX_BAYEUX_BAYEUX_RESPONSE_H_ */
