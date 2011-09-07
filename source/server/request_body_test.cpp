// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/system/error_code.hpp>
#include <boost/test/unit_test.hpp>

#include <vector>

#include "http/request.h"
#include "http/test_request_texts.h"
#include "server/connection.h"
#include "server/error.h"
#include "server/response.h"
#include "server/test_socket.h"
#include "server/traits.h"
#include "tools/io_service.h"
#include "tools/iterators.h"

namespace
{
	/*
	 * response implementation, that just reads the body
	 */
	template < class Connection >
	class read_body : public server::async_response, public boost::enable_shared_from_this< read_body< Connection > >
	{
	public:
		read_body( const http::request_header& request, const boost::shared_ptr< Connection >& connection )
			: connection_( connection )
			, body_read_( false )
			, has_body_( request.body_expected() )
		{
		}

		~read_body()
		{
			connection_->response_completed( *this );
		}

		/**
		 * @brief returns true, if the request contained a body and that body was completely read from the connection
		 */
		bool has_body() const
		{
			return has_body_;
		}

		/**
		 * @brief returns true, if the past buffer is equal to the received one.
		 */
		bool equal( const std::vector< char >& buffer ) const
		{
			return buffer == body_;
		}

		void body_read( const boost::system::error_code& error,
		  	  	        const char* 					 buffer,
		  	  	 	    std::size_t 					 bytes_read_and_decoded )
		{
			assert( !body_read_ );

			if ( error )
				throw std::runtime_error( "error while reading body ..." );

			body_.insert( body_.end(), buffer, buffer + bytes_read_and_decoded );

			if ( bytes_read_and_decoded )
	        	connection_->async_read_some_body( boost::bind( &read_body::body_read, this->shared_from_this() ) );
			else
				body_read_ = true;
		}

	private:
		// not implemented
		read_body( const read_body& );
		read_body& operator=( const read_body& );

        virtual void start()
        {
        	if ( !has_body_ )
        		connection_->response_completed( *this );
        	else
        		connection_->async_read_some_body( boost::bind( &read_body::body_read, this->shared_from_this() ) );
        }

        boost::shared_ptr< Connection >	connection_;
        bool							body_read_;
        bool							has_body_;
        std::vector< char > 			body_;
	};

	/*
	 * Factory creating read_body responses
	 */
	struct response_factory
	{
	    response_factory()
	    	: error_count_( 0 )
	    {
	    }

	    template < class T >
	    explicit response_factory( const T& )
			: error_count_( 0 )
		{
		}

	    template < class Connection >
	    boost::shared_ptr< server::async_response > create_response(
	        const boost::shared_ptr< Connection >&                    connection,
	        const boost::shared_ptr< const http::request_header >&    header )
	    {
	        const boost::shared_ptr< read_body< Connection > > new_response(new read_body< Connection >( *header, connection ) );
	        read_bodies_.push_back( new_response );

	        return boost::shared_ptr< server::async_response >( new_response );
	    }

	    template < class Connection >
        boost::shared_ptr< server::async_response > error_response(
        	const boost::shared_ptr< Connection >& 	con,
        	http::http_error_code					ec )
        {
	    	++error_count_;
            return boost::shared_ptr< server::async_response >( new ::server::error_response< Connection >( con, ec ) );
        }

	    std::vector< boost::shared_ptr< server::async_response > > 	read_bodies_;
	    int 														error_count_;
	};

	typedef server::test::socket<const char*>                       socket_t;
	typedef server::connection_traits< socket_t, response_factory > trait_t;
	typedef server::connection< trait_t >                           connection_t;
}

/**
 * @test small request body, most likely to be fetched already into the next request header buffer.
 */
BOOST_AUTO_TEST_CASE( post_with_small_message_body )
{
	trait_t					trait;
	boost::asio::io_service	queue;
	socket_t				socket( queue, tools::begin( http::test::simple_post ), tools::end( http::test::simple_post ) -1 );

	boost::shared_ptr< connection_t > connection( new connection_t( socket, trait ) );
	connection->start();

	tools::run( queue );
	BOOST_CHECK( trait.read_bodies_.size() == 1 );
	BOOST_CHECK( "needs to check the body content" == 0 );
}

/**
 * @test test that multiple successive bodies with different size will be received correctly
 */
BOOST_AUTO_TEST_CASE( multiple_bodies_with_different_size )
{
	BOOST_CHECK( false );
}

