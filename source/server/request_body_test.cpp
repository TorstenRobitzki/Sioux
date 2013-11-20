#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/system/error_code.hpp>
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <iostream>
#include <vector>

#include "http/request.h"
#include "http/test_request_texts.h"
#include "asio_mocks/test_socket.h"
#include "asio_mocks/test_timer.h"
#include "server/connection.h"
#include "server/error.h"
#include "server/log.h"
#include "server/response.h"
#include "server/test_tools.h"
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
		    , has_error_( false )
		{
		}

		/**
		 * @brief returns true, if the request contained a body and that body was completely read from the connection
		 */
		bool has_body() const
		{
			return has_body_;
		}

		/**
		 * @brief returns true, if the passed buffer is equal to the received one.
		 */
		template < class Iter >
		bool equal( Iter begin, Iter end ) const
		{
			return body_read_ && server::test::compare_buffers( std::vector< char >( begin, end ), body_, std::cerr );
		}

		/**
		 * @copydoc template < class Iter > bool equal( Iter begin, Iter end ) const
		 */
		bool equal( const std::vector< char >& buffer ) const
		{
			return body_read_ && server::test::compare_buffers( buffer, body_, std::cerr );
		}

		/**
		 * @copydoc template < class Iter > bool equal( Iter begin, Iter end ) const
		 */
		template < std::size_t N >
		bool equal( const char ( &buffer )[ N ] ) const
		{
			return equal( tools::begin( buffer ), tools::end( buffer ) - 1 );
		}

		void body_read( const boost::system::error_code& error,
		  	  	        const char* 					 buffer,
		  	  	 	    std::size_t 					 bytes_read_and_decoded )
		{
		    assert( !body_read_ );

			if ( error )
			{
				connection_->response_completed( *this );
				has_error_ = true;
				return;
			}

			body_.insert( body_.end(), buffer, buffer + bytes_read_and_decoded );

			if ( !bytes_read_and_decoded )
			{
				connection_->response_completed( *this );
				body_read_ = true;
			}
		}

		bool body_completed() const
		{
			return body_read_;
		}

		bool has_error() const
		{
		    return has_error_;
		}

		std::size_t body_size() const
		{
		    return body_.size();
		}
	private:
		// not implemented
		read_body( const read_body& );
		read_body& operator=( const read_body& );

        virtual void start()
        {
        	if ( !has_body_ )
        	{
        		connection_->response_completed( *this );
        	}
        	else
        	{
        		connection_->async_read_body(
        				boost::bind( &read_body::body_read, this->shared_from_this(), _1, _2, _3 ) );
        	}
        }

        virtual const char* name() const
        {
            return "request_body_test.cpp::read_body";
        }

        boost::shared_ptr< Connection >	connection_;
        bool							body_read_;
        bool							has_body_;
        std::vector< char > 			body_;
        bool							has_error_;
	};

	typedef std::vector< boost::shared_ptr< server::async_response > > response_list_t;

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
	    	if ( header->state() == http::message::ok )
	    	{
				const boost::shared_ptr< read_body< Connection > > new_response(new read_body< Connection >( *header, connection ) );
				read_bodies_.push_back( new_response );

				return boost::shared_ptr< server::async_response >( new_response );
	    	}

	    	return boost::shared_ptr< server::async_response >(
	    			new server::error_response< Connection >( connection, http::http_bad_request ) );
	    }

	    template < class Connection >
        boost::shared_ptr< server::async_response > error_response(
        	const boost::shared_ptr< Connection >& 	con,
        	http::http_error_code					ec )
        {
	    	++error_count_;
            return boost::shared_ptr< server::async_response >( new ::server::error_response< Connection >( con, ec ) );
        }

	    response_list_t		read_bodies_;
	    int 				error_count_;
	};

	typedef asio_mocks::socket<const char*>                         socket_t;
	typedef boost::asio::deadline_timer                             timer_t;
	typedef server::null_event_logger								event_logger_t;
//	typedef server::stream_event_log								event_logger_t;
	typedef server::stream_error_log								error_logger_t;

	struct trait_t : server::connection_traits< socket_t, timer_t, response_factory, event_logger_t, error_logger_t >
	{
		typedef server::connection_traits< socket_t, timer_t, response_factory, event_logger_t, error_logger_t > base_t;

		explicit trait_t() : base_t( *this ) {}

		std::ostream& logstream() const
		{
			return std::cerr;
		}

		/*
		 * The connection is generating events in the d'tor. So all connections are destroyed before, the loggers
		 * d'tor is called.
		 */
		~trait_t()
		{
			read_bodies_.clear();
		}
	};

	typedef server::connection< trait_t >                           connection_t;

	read_body< connection_t >& get_body( const boost::shared_ptr< server::async_response >& response )
	{
		return dynamic_cast< read_body< connection_t >& >( *response );
	}

	template < class Iter >
	std::vector< char > build_randomly_chunked_post_request( boost::minstd_rand& random, Iter begin, Iter end,
			std::size_t max_chunk_size )
	{
		static const char header[] =
			"POST / HTTP/1.1\r\n"
			"Host: web-sniffer.net\r\n"
			"Origin: http://web-sniffer.net\r\n"
			"User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_6_8) AppleWebKit/534.50 (KHTML, like Gecko) Version/5.1 Safari/534.50\r\n"
			"Content-Type: application/x-www-form-urlencoded\r\n"
			"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
			"Referer: http://web-sniffer.net/\r\n"
			"Transfer-Encoding: chunked\r\n"
			"Accept-Language: de-de\r\n"
			"Accept-Encoding: gzip, deflate\r\n\r\n";

		std::vector< char > message = server::test::random_chunk( random, std::vector< char >( begin, end ), max_chunk_size );
		message.insert( message.begin(), tools::begin( header ), tools::end( header ) -1 );

		return message;
	}
}

/**
 * @class server::connection
 * @test small request body, most likely to be fetched already into the next request header buffer.
 *
 * The length of the body is encoded with a Content-Length header.
 */
BOOST_AUTO_TEST_CASE( post_with_small_content_length_message_body )
{
	trait_t					trait;
	boost::asio::io_service	queue;
	socket_t				socket( queue, tools::begin( http::test::simple_post ),
								tools::end( http::test::simple_post ) -1 );

	boost::shared_ptr< connection_t > connection( new connection_t( socket, trait ) );
	connection->start();

	tools::run( queue );
	BOOST_CHECK_EQUAL( 1u, trait.read_bodies_.size() );
	BOOST_CHECK( get_body( trait.read_bodies_.front() ).body_completed() );
	BOOST_CHECK( get_body( trait.read_bodies_.front() )
		.equal( "url=http%3A%2F%2Fasdasdasd&submit=Submit&http=1.1&gzip=yes&type=GET&uak=0" ) );
}

/**
 * @class server::connection
 * @test chunked encoded message body to be received and decoded
 * @todo implement
 */
BOOST_AUTO_TEST_CASE( post_with_small_chunked_encoded_message_body )
{
/*
    boost::minstd_rand      random;
    const char body[] = "Es war einmal ein Baer der schwamm so weit im Meer.";

    const std::vector< char > message = build_randomly_chunked_post_request(
    		random, tools::begin( body ), tools::end( body ), 7u );

	trait_t					trait;
	boost::asio::io_service	queue;
	socket_t				socket( queue, &message[0], &message[message.size()] );

	boost::shared_ptr< connection_t > connection( new connection_t( socket, trait ) );
	connection->start();

	tools::run( queue );
	BOOST_CHECK_EQUAL( trait.read_bodies_.size(), 1u );
	BOOST_CHECK( get_body( trait.read_bodies_.front() ).equal( body ) );
	*/
}

/**
 * @class server::connection
 * @test multiple request bodies
 */
BOOST_AUTO_TEST_CASE( post_with_multiple_small_content_length_message_body )
{
	static const std::size_t	number_of_bodies = 100;

	trait_t					trait;
	boost::asio::io_service	queue;
	socket_t				socket( queue, tools::begin( http::test::simple_post ),
									tools::end( http::test::simple_post ) -1, 0, number_of_bodies );

	boost::shared_ptr< connection_t > connection( new connection_t( socket, trait ) );
	connection->start();

	tools::run( queue );
	BOOST_CHECK_EQUAL( trait.read_bodies_.size(), number_of_bodies );

	for ( response_list_t::const_iterator response = trait.read_bodies_.begin();
			response != trait.read_bodies_.end(); ++response )
	{
		BOOST_REQUIRE( get_body( *response ).equal(
			"url=http%3A%2F%2Fasdasdasd&submit=Submit&http=1.1&gzip=yes&type=GET&uak=0" ) );
	}
}

/**
 * @class server::connection
 * @test multiple request bodies, delivered in very small chunks.
 */
BOOST_AUTO_TEST_CASE( post_with_multiple_small_content_length_message_body_read_in_small_chunks )
{
	static const std::size_t	number_of_bodies = 100;
	static const std::size_t	chunk_size		 = 10;

	trait_t					trait;
	boost::asio::io_service	queue;
	socket_t				socket( queue, tools::begin( http::test::simple_post ),
									tools::end( http::test::simple_post ) -1, chunk_size, number_of_bodies );

	boost::shared_ptr< connection_t > connection( new connection_t( socket, trait ) );
	connection->start();

	tools::run( queue );
	BOOST_CHECK_EQUAL( trait.read_bodies_.size(), number_of_bodies );

	for ( response_list_t::const_iterator response = trait.read_bodies_.begin();
			response != trait.read_bodies_.end(); ++response )
	{
		BOOST_REQUIRE( get_body( *response ).equal(
			"url=http%3A%2F%2Fasdasdasd&submit=Submit&http=1.1&gzip=yes&type=GET&uak=0" ) );
	}
}

/**
 * @class server::connection
 * @test multiple request bodies, delivered in very small chunks.
 */
BOOST_AUTO_TEST_CASE( post_with_multiple_small_content_length_message_body_read_in_one_hugh_chunk )
{
	static const std::size_t	number_of_bodies = 100;

	std::vector< char > big_message;
	for ( int i = 0; i != number_of_bodies; ++i )
		big_message.insert( big_message.end(),
				tools::begin( http::test::simple_post ), tools::end( http::test::simple_post ) -1 );

	trait_t					trait;
	boost::asio::io_service	queue;
	socket_t				socket( queue, &big_message[0], &big_message[0] + big_message.size() );

	boost::shared_ptr< connection_t > connection( new connection_t( socket, trait ) );
	connection->start();

	tools::run( queue );
	BOOST_CHECK_EQUAL( trait.read_bodies_.size(), number_of_bodies );

	for ( response_list_t::const_iterator response = trait.read_bodies_.begin();
			response != trait.read_bodies_.end(); ++response )
	{
		BOOST_REQUIRE( get_body( *response ).equal(
			"url=http%3A%2F%2Fasdasdasd&submit=Submit&http=1.1&gzip=yes&type=GET&uak=0" ) );
	}
}

/**
 * @class server::connection
 * @test mixing requests with and without body should result in correct delivering of the message headers and bodies
 * @todo implement
 */
BOOST_AUTO_TEST_CASE( mixing_multiple_request_with_and_without_body )
{
}

/**
 * @class server::connection
 * @test test that multiple successive bodies with different size will be received correctly
 * @todo implement
 */
BOOST_AUTO_TEST_CASE( multiple_bodies_with_different_size )
{
	const char message_header[] =
			"POST / HTTP/1.1\r\n"
	    	"Host: web-sniffer.net\r\n"
	    	"Origin: http://web-sniffer.net\r\n"
	    	"Content-Type: application/x-www-form-urlencoded\r\n"
	    	"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
	    	"Referer: http://web-sniffer.net/\r\n"
	    	"Accept-Encoding: gzip, deflate\r\n"
	    	"Content-Length: ";

	using namespace server;

	boost::minstd_rand		random;
	const unsigned			number_of_bodies = 1000;
	const std::size_t		max_body_size    = 10 * 1024;

	std::vector< char >		all_messages;
	std::vector< std::vector< char > > bodies;

	// generate number_of_bodies messages at once
	for ( unsigned n = 0; n != number_of_bodies; ++n )
	{
	    typedef boost::uniform_int<std::size_t> distribution_type;
	    typedef boost::variate_generator<boost::minstd_rand&, distribution_type> gen_type;

	    distribution_type distribution( 1, max_body_size );
	    gen_type die_gen(random, distribution);

	    const std::vector< char > new_body = test::random_body( random, die_gen() );
	    const std::string 			length = tools::as_string( new_body.size() ) + "\r\n\r\n";

	    all_messages.insert( all_messages.end(), tools::begin( message_header ) , tools::end( message_header ) -1 );
	    all_messages.insert( all_messages.end(), length.begin() , length.end() );
	    all_messages.insert( all_messages.end(), new_body.begin() , new_body.end() );
	}

	trait_t					trait;
	boost::asio::io_service	queue;
	socket_t				socket( queue, &all_messages[0], &all_messages[0] + all_messages.size(),
									random, 1, 2 * max_body_size );

	boost::shared_ptr< connection_t > connection( new connection_t( socket, trait ) );
	connection->start();

	tools::run( queue );

	BOOST_REQUIRE_EQUAL( number_of_bodies, trait.read_bodies_.size() );
}

/**
 * @class server::connection
 * @test a missing or incomplete body must result in an error being detected and the connection getting closed
 */
BOOST_AUTO_TEST_CASE( incomplete_request_body )
{
	trait_t					trait;
	boost::asio::io_service	queue;
	socket_t				socket( queue, tools::begin( http::test::simple_post ),
								tools::end( http::test::simple_post ) -2 ); // one byte missing

	boost::shared_ptr< connection_t > connection( new connection_t( socket, trait ) );
	connection->start();

	tools::run( queue );

	BOOST_REQUIRE_EQUAL( 1u, trait.read_bodies_.size() );
	BOOST_CHECK( !get_body( trait.read_bodies_.front() ).body_completed() );
}

/**
 * @class server::connection
 * @test an error occurs while reading a body
 */
BOOST_AUTO_TEST_CASE( error_while_reading_length_encoded_body )
{
	trait_t					trait;
	boost::asio::io_service	queue;
	const std::size_t		message_lenght =
			std::distance( tools::begin( http::test::simple_post ), tools::end( http::test::simple_post ) ) - 1;

	socket_t				socket( queue, tools::begin( http::test::simple_post ),
								tools::end( http::test::simple_post ) -1, make_error_code( server::limit_reached ),
								message_lenght - 5u, boost::system::error_code(), 10000u );

	boost::shared_ptr< connection_t > connection( new connection_t( socket, trait ) );
	connection->start();

	tools::run( queue );
	BOOST_REQUIRE_EQUAL( 1u, trait.read_bodies_.size() );
	BOOST_CHECK( !get_body( trait.read_bodies_.front() ).body_completed() );
}

/**
 * @class server::connection
 * @test timeout while receiving a body
 */
BOOST_AUTO_TEST_CASE( timeout_while_receiving_a_request_body )
{
	trait_t					trait;
	boost::asio::io_service	queue;

	asio_mocks::read_plan plan;
	plan
		<< asio_mocks::read( tools::begin( http::test::simple_post ),	tools::end( http::test::simple_post ) -5 )
		<< asio_mocks::delay( boost::posix_time::seconds( 10 ) )
		<< asio_mocks::read( tools::end( http::test::simple_post ) -5, tools::end( http::test::simple_post ) -1 );

	socket_t				socket( queue, plan );

	boost::shared_ptr< connection_t > connection( new connection_t( socket, trait ) );
	connection->start();

	tools::run( queue );
	BOOST_REQUIRE_EQUAL( 1u, trait.read_bodies_.size() );
	BOOST_CHECK( !get_body( trait.read_bodies_.front() ).body_completed() );
}

/**
 * @test in case, that the body receiving response removes it self from the connection (by calling response_completed()
 * or response_not_possible()), no further calls to the read-body handler should happen.
 * @todo implement
 */
BOOST_AUTO_TEST_CASE( no_further_body_read_callbacks_after_stop_responding )
{
}

/**
 * @test if the body of the request is expected, but missing, an installed read handler should be called once.
 */
BOOST_AUTO_TEST_CASE( missing_body_should_be_flagged_as_error )
{
    const char simple_post_with_missing_body[] =
        "POST / HTTP/1.1\r\n"
        "Host: web-sniffer.net\r\n"
        "Origin: http://web-sniffer.net\r\n"
        "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_6_8) AppleWebKit/534.50 (KHTML, like Gecko) Version/5.1 Safari/534.50\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
        "Referer: http://web-sniffer.net/\r\n"
        "Accept-Language: de-de\r\n"
        "Accept-Encoding: gzip, deflate\r\n"
        "Content-Length: 73\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";

    trait_t                 trait;
    boost::asio::io_service queue;

    asio_mocks::read_plan plan;
    plan
        << asio_mocks::read(
            tools::begin( simple_post_with_missing_body ), tools::end( simple_post_with_missing_body ) -1 )
        << asio_mocks::disconnect_read();

    socket_t                socket( queue, plan );

    boost::shared_ptr< connection_t > connection( new connection_t( socket, trait ) );
    connection->start();

    tools::run( queue );
    BOOST_REQUIRE_EQUAL( 1u, trait.read_bodies_.size() );
    BOOST_CHECK( get_body( trait.read_bodies_.front() ).has_error() );
    BOOST_CHECK_EQUAL( 0, get_body( trait.read_bodies_.front() ).body_size() );
}

/**
 * @test make sure, an empty body will issue no read, but will call the completition handler
 * The example is taken from an Safari Ajax HTML DELETE
 */
BOOST_AUTO_TEST_CASE( empty_body_should_result_in_callback_beeing_called )
{
    const char delete_with_empty_body[] =
        "DELETE /messages/623 HTTP/1.1\r\n"
        "Host: 127.0.0.1:8080\r\n"
        "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_8_3) AppleWebKit/536.28.10 (KHTML, like Gecko) Version/6.0.3 Safari/536.28.10\r\n"
        "Content-Length: 0\r\n"
        "Accept: */*\r\n"
        "Origin: http://127.0.0.1:8080\r\n"
        "X-CSRF-Token: BXrs6yvcoyx8E7U43FhWXf7dfA3+RG3OX843qi7oFyQ=\r\n"
        "X-Requested-With: XMLHttpRequest\r\n"
        "Referer: http://127.0.0.1:8080/home\r\n"
        "DNT: 1\r\n"
        "Accept-Language: de-de\r\n"
        "Accept-Encoding: gzip, deflate\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";

    trait_t                 trait;
    boost::asio::io_service queue;

    asio_mocks::read_plan plan;
    plan
        << asio_mocks::read(
            tools::begin( delete_with_empty_body ), tools::end( delete_with_empty_body ) -1 )
        << asio_mocks::disconnect_read();

    socket_t                socket( queue, plan );

    boost::shared_ptr< connection_t > connection( new connection_t( socket, trait ) );
    connection->start();

    tools::run( queue );
    BOOST_REQUIRE_EQUAL( 1u, trait.read_bodies_.size() );
    BOOST_CHECK( !get_body( trait.read_bodies_.front() ).has_error() );
    BOOST_CHECK_EQUAL( 0, get_body( trait.read_bodies_.front() ).body_size() );
}
