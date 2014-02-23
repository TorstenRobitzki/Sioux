// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SOURCE_SIOUX_CONNECTION_H
#define SOURCE_SIOUX_CONNECTION_H

#include "http/body_decoder.h"
#include "http/http.h"
#include "http/request.h"
#include "server/error_code.h"
#include "server/response.h"
#include "server/timeout.h"
#include "tools/mem_tools.h"
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/write.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <algorithm>
#include <cassert>
#include <deque>
#include <map>
#include <memory>

/** @namespace server */
namespace server 
{
    class async_response;

	/**
	 * @brief representation of a http connection over a physical connection from a client to this server
	 *
	 * It's the responsibility of the connection object to parse incoming requests
	 * and to coordinate outgoing responses.
     *
     * The connection class is not responsible for http persistent connection support,
     * this have to be implemented within the responses.
     *
     * The given Trait class has to have a member function keep_alive_timeout(), that returns a 
     * boost::posix_time::time_duration, that gives the maximum time, that a connection stays
     * open and idle, until it's closed. A connection is idle, when no response is pending
     * and when a new header is read.
     *
     * The given Trait class has to have a member function timeout(), that returns
     * a boost::posix_time::time_duration, that gives the maximum time, a read or write can
     * last until the connection is closed.
	 */
    template < class Trait,
               class Connection = typename Trait::network_stream_type,
               class Timer      = typename Trait::timeout_timer_type >
    class connection : public boost::enable_shared_from_this< connection< Trait, Connection, Timer > >
	{
	public:
        typedef Connection  socket_t;
        typedef Timer       timer_t;
        typedef Trait       trait_t;

        /**
         * @brief contructs a connection object by passing an IO object and trait object.
         */
        template <class ConArg>
		connection(ConArg Arg, Trait& trait);

        ~connection();

        /**
         * @brief starts the first asynchron read on the connection passed to the c'tor
         */
        void start();

        /**
         * @brief interface for writing to the connection
         *
         * Writes to the underlying connection with the configured timeout. It's important to keep in mind, that
         * there can be several async_response that want to send and that it might take some time until it's up the
         * sender to get it's response send.
         *
         * timeout() is called for the trait object that was passed to the c'tor to determine the timeout value. The
         * timeout is only applied, if the connection is really used for sending, not when waiting for the response of
         * request that where made earlier.
         *
         * @post a async_response implementation have to call response_completed() or response_not_possible() once after
         *       calling async_write_some() or async_write() the last time
         */
        template<
            typename ConstBufferSequence,
            typename WriteHandler>
        void async_write_some(
            const ConstBufferSequence&  buffers,
            WriteHandler                handler,
            async_response&             sender);

        template<
            typename ConstBufferSequence,
            typename WriteHandler>
        void async_write(
            const ConstBufferSequence&  buffers,
            WriteHandler                handler,
            async_response&             sender);

        /**
         * @brief Starts reading asynchronously the body of a request.
         *
         * The passed handler will be called until the whole body is decoded and passed to the handler or until
         * an exception is thrown from the handler.
         * If the read handler is called with a bytes_read_and_decoded of 0, the complete body is read.
         *
		 * The body is free from any encoding. The handler must have following signature:
		 * void handler(
  	  	 *		const boost::system::error_code& error, // Result of operation.
  	  	 *		const char* buffer,						// buffer, containing the read and decoded body
  	  	 * 		std::size_t bytes_read_and_decoded      // Number of bytes received.
  	  	 * 		);
  	  	 *
  	  	 * @pre last received header signaled, that a body is expected
         */
        template< typename ReadHandler >
        void async_read_body( ReadHandler handler );

        /**
         * @brief to be called by an async_response to signal, that no more writes will be done
         *
         * Usually this function will be called by a async_response implementations d'tor. This
         * have to be called exactly once by an async_response that called response_started() before.
         *
         * It must be harmless to call this function, even when response_not_possible() was called before,
         * or otherwise the async_response implementations have to keep track if an error occurred.
         */
        void response_completed(async_response& sender);

        /**
         * @brief end of response with error
         *
         * Same as response_completed(), but with an error code. The connection will do the necessary
         * response to the client (if possible).
         *
         * In case of a http_internal_server_error error code, all running requests are canceled and the underlying
         * connection will be closed.
         */
        void response_not_possible(async_response& sender, http::http_error_code );

        void response_not_possible(async_response& sender);

        /**
         * @brief closes the reading part of the used connection
         */
        void shutdown_read();

        /**
         * @brief closes the writing part of the connection and if not already done, the reading part too.
         */
        void shutdown_close();

        /**
         * @brief returns a reference to the underlying connection. Used for the construction phase only.
         */
        socket_t& socket();

        Trait& trait();
	private:
        // not implemented
        connection( const connection& );
        connection& operator=( const connection& );

        class blocked_write_base 
        {
        public:
            virtual ~blocked_write_base() {}

            virtual void operator()(Connection&) = 0;

            virtual void cancel() = 0;
        };

        template <class ConstBufferSequence, class WriteHandler>
        class blocked_write : public blocked_write_base
        {
        public:
            blocked_write(const ConstBufferSequence&  buffers,
                          WriteHandler                handler);
        private:
            void operator()(Connection&);
            void cancel();

            const ConstBufferSequence   buffers_;
            const WriteHandler          handler_;
        };

        template <class ConstBufferSequence, class WriteHandler>
        class blocked_write_some : public blocked_write_base
        {
        public:
            blocked_write_some(const ConstBufferSequence&  buffers,
                          WriteHandler                handler);
        private:
            void operator()(Connection&);
            void cancel();

            const ConstBufferSequence   buffers_;
            const WriteHandler          handler_;
        };

        boost::posix_time::time_duration read_timeout_value() const;
        void issue_read(const boost::posix_time::time_duration& time_out);
		void handle_read(const boost::system::error_code& e, std::size_t bytes_transferred);
		void handle_keep_alive_timeout( const boost::system::error_code& ec );

		/*
         * handles a newly read request_header and returns true, if further request_header should be 
         * read
         */
        bool handle_request_header(const boost::shared_ptr<const http::request_header>& new_request);

        template <class Handler>
        void unblock_pending_writes(async_response& sender, const Handler&);

        void response_not_possible_impl(async_response& sender, const boost::shared_ptr<async_response>& error_response);

        // hurries all responses, that are in front of the given sender
        void hurry_writers(async_response& sender);

        void deliver_body();

        Connection                              connection_;
        Trait&                                  trait_;

        boost::shared_ptr<http::request_header> current_request_;

        typedef std::deque<async_response*>     response_list;
        response_list                           responses_;

        typedef std::map<async_response*, std::vector<blocked_write_base*> >  blocked_write_list;
        blocked_write_list                      blocked_writes_;

        bool                                    current_response_is_sending_;
        bool                                    shutdown_read_;
        bool                                    no_read_timeout_set_;

        timer_t                                 read_timer_;
        timer_t                                 write_timer_;

        typedef boost::function< void ( const boost::system::error_code&, const char*, std::size_t ) >
        	body_read_cb_t;

        http::body_decoder						body_decoder_;
        // if not empty(), currently, a body is read
        body_read_cb_t							body_read_call_back_;
        std::vector< char >						body_buffer_;
 	};

    /**
     * @brief creates a new connection object and calls start() on it.
     * @relates connection
     */
	template <class Connection, class Trait>
    boost::shared_ptr<connection<Trait, Connection> > create_connection(const Connection& con, Trait& trait);

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // implementation
	template < class Trait, class Connection, class Timer >
    template < class ConArg >
	connection< Trait, Connection, Timer >::connection(ConArg arg, Trait& trait)
        : connection_(arg)
        , trait_(trait)
        , current_request_()
        , current_response_is_sending_(false)
        , shutdown_read_(false)
        , no_read_timeout_set_( false )
        , read_timer_(connection_.get_io_service())
        , write_timer_(connection_.get_io_service())
        , body_decoder_()
        , body_read_call_back_()
    {
        trait_.event_connection_created(*this);
    }

    template < class Trait, class Connection, class Timer >
	connection< Trait, Connection, Timer >::~connection()
    {
        assert( blocked_writes_.empty() );
        assert( body_read_call_back_.empty() );
        assert( responses_.empty() );
    
        boost::system::error_code ec;
        connection_.close( ec );

        trait_.event_connection_destroyed( *this );
    }

    template < class Trait, class Connection, class Timer >
    void connection< Trait, Connection, Timer >::start()
    {
        current_request_.reset( new http::request_header );
        issue_read( trait_.timeout() );
    }

    template < class Trait, class Connection, class Timer >
    void connection< Trait, Connection, Timer >::hurry_writers(async_response& sender)
    {
        // hurry resposes that are blocking the sender
        for ( response_list::const_iterator r = responses_.begin(); *r != &sender; ++r )
        {
            assert( r != responses_.end() );
            assert( *r );
            (*r)->hurry();
        }
    }

    template < class Trait, class Connection, class Timer >
    void connection< Trait, Connection, Timer >::deliver_body()
    {
    	assert( !body_read_call_back_.empty() );
    	for ( std::pair< std::size_t, const char* > current = body_decoder_.decode(); current.first;
    			current = body_decoder_.decode() )
    	{
    		body_read_call_back_( boost::system::error_code(), current.second, current.first );
    	}
    }

    template < class Trait, class Connection, class Timer >
    template<typename ConstBufferSequence, typename WriteHandler>
    void connection< Trait, Connection, Timer >::async_write(
        const ConstBufferSequence&  buffers,
        WriteHandler                handler,
        async_response&             sender)
    {
        assert( !responses_.empty() );
        trait_.event_data_write(*this, buffers, sender);

        if ( responses_.front() == &sender )
        {
            server::async_write_with_to(
                connection_,
                buffers,
                handler,
                write_timer_,
                trait_.timeout() );

            current_response_is_sending_ = true;
        }
        else
        {
            trait_.event_writer_blocked(*this, buffers, sender);
            hurry_writers(sender);

            // store send request until the current sender is ready
            typename blocked_write_list::mapped_type& writes = blocked_writes_[&sender];
            tools::save_push_back(new blocked_write<ConstBufferSequence, WriteHandler>(buffers, handler), writes);
        }
    }

    template < class Trait, class Connection, class Timer >
    template<typename ConstBufferSequence, typename WriteHandler>
    void connection< Trait, Connection, Timer >::async_write_some(
        const ConstBufferSequence&  buffers,
        WriteHandler                handler,
        async_response&             sender)
    {
        trait_.event_data_write(*this, buffers, sender);

        if ( responses_.front() == &sender )
        {
            server::async_write_with_to(
                connection_,
                buffers,
                handler,
                write_timer_,
                trait_.timeout() );

            current_response_is_sending_ = true;
        }
        else
        {
            trait_.event_writer_blocked(*this, buffers, sender);

            hurry_writers(sender);

            // store send request until the current sender is ready
            typename blocked_write_list::mapped_type& writes = blocked_writes_[&sender];
            tools::save_push_back(new blocked_write_some<ConstBufferSequence, WriteHandler>(buffers, handler), writes);
        }
    }

    template < class Trait, class Connection, class Timer >
    template< typename ReadHandler >
    void connection< Trait, Connection, Timer >::async_read_body( ReadHandler handler )
    {
		assert( current_request_->body_expected() );
		assert( body_read_call_back_.empty() );

		body_decoder_.start( *current_request_ );

		if ( body_decoder_.done() )
		{
            const boost::system::error_code no_error;
            connection_.get_io_service().post( boost::bind< void >( handler, no_error, static_cast< const char* >( 0 ), 0 ) );
		}
		else
		{
	        body_read_call_back_ = handler;
		}

    }

    template < class Trait, class Connection, class Timer >
    void connection< Trait, Connection, Timer >::response_completed(async_response& sender)
    {
        trait_.event_response_completed(*this, sender);

        // there is no reason, why there should be outstanding, blocked writes from the current sender
        assert(blocked_writes_.erase(&sender) == 0);
        
        response_list::iterator senders_pos = std::find(responses_.begin(), responses_.end(), &sender);

        if ( senders_pos == responses_.begin() &&  senders_pos != responses_.end() )
        {
            current_response_is_sending_ = false;
            responses_.pop_front();

            if ( !responses_.empty() )
            {
                const typename blocked_write_list::iterator pending_writes = blocked_writes_.find(responses_.front());

                if ( pending_writes != blocked_writes_.end() )
                {
                    // first remove all pending writes from the list
                    typename blocked_write_list::mapped_type                             list;
                    tools::ptr_container_guard<typename blocked_write_list::mapped_type> delete_blocked_writes(list);

                    list.swap(pending_writes->second);
                    blocked_writes_.erase(pending_writes);

                    std::for_each(list.begin(), list.end(),
                        boost::bind(&blocked_write_base::operator(), _1, boost::ref(connection_)));
                }
            }
        }
        else if ( senders_pos != responses_.end() )
        {
            responses_.erase(senders_pos);
        }

        // start keep timeout, if the last read was issued without timeout
        if ( responses_.empty() && no_read_timeout_set_ && !shutdown_read_ )
        {
            read_timer_.expires_from_now( trait_.keep_alive_timeout() );
            read_timer_.async_wait(
                boost::bind( &connection::handle_keep_alive_timeout,
                             boost::static_pointer_cast< connection< Trait, Connection > >( this->shared_from_this() ),
                             boost::asio::placeholders::error ) );
        }
    }

    template < class Trait, class Connection, class Timer >
    void connection< Trait, Connection, Timer >::response_not_possible_impl(async_response& sender, const boost::shared_ptr<async_response>& error_response)
    {
        const response_list::iterator senders_pos = std::find(responses_.begin(), responses_.end(), &sender);
        assert( senders_pos != responses_.end() );

        if ( senders_pos == responses_.end() -1 )
        {
            body_read_call_back_.clear();
        }

        const typename blocked_write_list::const_iterator writes = blocked_writes_.find(&sender);

        if ( writes != blocked_writes_.end() )
        {
            tools::ptr_container_guard<typename blocked_write_list::mapped_type> guard(writes->second);
            std::for_each(writes->second.begin(), writes->second.end(), 
                boost::bind(&blocked_write_base::cancel, _1));
        }
 
        if ( error_response.get() )
        {
            *senders_pos = error_response.get();
            error_response->start();
        }
        else
        {
            responses_.erase(senders_pos);
            shutdown_close();
        }
    }

    template < class Trait, class Connection, class Timer >
    void connection< Trait, Connection, Timer >::response_not_possible(async_response& sender, http::http_error_code ec)
    {
        trait_.event_response_not_possible(*this, sender, ec);

        boost::shared_ptr<async_response> error_response;

        // if no data was send from this response, consult the error-trait to get an error response
        if ( responses_.front() != &sender || !current_response_is_sending_ )
            error_response = trait_.error_response( this->shared_from_this(), ec );

        response_not_possible_impl(sender, error_response);
    }

    template < class Trait, class Connection, class Timer >
    void connection< Trait, Connection, Timer >::response_not_possible(async_response& sender)
    {
        trait_.event_response_not_possible(*this, sender);

        response_not_possible_impl(sender, boost::shared_ptr<async_response>());
    }

    template < class Trait, class Connection, class Timer >
    void connection< Trait, Connection, Timer >::shutdown_read()
    {
        if ( !shutdown_read_ )
        {
            trait_.event_shutdown_read(*this);
            shutdown_read_ = true;

            boost::system::error_code ec;
            connection_.shutdown( boost::asio::ip::tcp::socket::shutdown_receive, ec );

            if ( ec )
                trait_.log_error( *this, "connection::shutdown_read", ec );
        }
    }

    template < class Trait, class Connection, class Timer >
    void connection< Trait, Connection, Timer >::shutdown_close()
    {
        trait_.event_shutdown_close(*this);

        shutdown_read();

        boost::system::error_code ec;
        connection_.shutdown( boost::asio::ip::tcp::socket::shutdown_send, ec );

        if ( ec )
            trait_.log_error( *this, "connection::shutdown_close", "calling shutdown", ec );

        boost::system::error_code close_ec;
        connection_.close( close_ec );

        if ( close_ec )
            trait_.log_error( *this, "connection::shutdown_close", "calling close", ec );
    }

    template < class Trait, class Connection, class Timer >
    Connection& connection< Trait, Connection, Timer >::socket()
    {
        return connection_;
    }

    template < class Trait, class Connection, class Timer >
    Trait& connection< Trait, Connection, Timer >::trait()
    {
        return trait_;
    }

    template < class Trait, class Connection, class Timer >
    boost::posix_time::time_duration connection< Trait, Connection, Timer >::read_timeout_value() const
    {
        boost::posix_time::time_duration result;

        if ( !current_request_->empty() || !body_read_call_back_.empty() )
            result = trait_.timeout();

        else if  ( responses_.empty() && current_request_->empty() && body_read_call_back_.empty() )
            result = trait_.keep_alive_timeout();

        return result;
    }

    template < class Trait, class Connection, class Timer >
    void connection< Trait, Connection, Timer >::issue_read( const boost::posix_time::time_duration& time_out )
    {
		std::pair< char*, std::size_t > buffer( 0, 0 );

		if ( body_read_call_back_.empty() )
		{
			buffer = current_request_->read_buffer();
			assert( buffer.first && buffer.second );
		}
		else
		{
			body_buffer_.resize( 1024u );
			buffer = std::make_pair( &body_buffer_[0], body_buffer_.size() );
		}

		if ( time_out != boost::posix_time::seconds( 0 ) )
		{
            server::async_read_some_with_to(
                connection_,
                boost::asio::buffer( buffer.first, buffer.second ),
                boost::bind( &connection::handle_read,
                             boost::static_pointer_cast< connection< Trait, Connection > >( this->shared_from_this() ),
                             boost::asio::placeholders::error,
                             boost::asio::placeholders::bytes_transferred ),
                read_timer_,
                time_out );

            no_read_timeout_set_ = false;
		}
		else
		{
		    connection_.async_read_some(
		        boost::asio::buffer( buffer.first, buffer.second ),
                boost::bind( &connection::handle_read,
                             boost::static_pointer_cast< connection< Trait, Connection > >( this->shared_from_this() ),
                             boost::asio::placeholders::error,
                             boost::asio::placeholders::bytes_transferred ) );

		    no_read_timeout_set_ = true;
		}
    }

    template < class Trait, class Connection, class Timer >
	void connection< Trait, Connection, Timer >::handle_read(const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        // cancel keep alive timer
        if ( no_read_timeout_set_ )
        {
            no_read_timeout_set_ = false;
            read_timer_.cancel();
        }

		if ( error || bytes_transferred == 0 )
		{
        	if ( !body_read_call_back_.empty() )
        	{
        		const boost::system::error_code published_error = error ? error : make_error_code( canceled_by_error );
        		body_read_call_back_( published_error, 0, 0 );
        		body_read_call_back_.clear();
        	}
			return;
		}

        while ( bytes_transferred != 0 )
        {
        	// reading a body
        	if ( !body_read_call_back_.empty() )
        	{
        		assert( !body_decoder_.done() );

        		const std::size_t decoded_size = body_decoder_.feed_buffer( &body_buffer_[0], bytes_transferred );

        		if ( decoded_size )
        		{
        			bytes_transferred -= decoded_size;
        			deliver_body();
        		}

        		if ( body_decoder_.done() )
        		{
        			body_buffer_.erase( body_buffer_.begin(), body_buffer_.begin() + decoded_size );
        			// this consumes and decreases bytes_transferred
					current_request_.reset( new http::request_header(
							boost::asio::const_buffers_1( &body_buffer_[0], bytes_transferred ), bytes_transferred ) );

					body_read_cb_t read_call_back;
                    body_read_call_back_.swap( read_call_back );

                    if ( !read_call_back.empty() )
                        read_call_back( error, 0, 0 );
        		}
        	}
        	// or reading a header
        	else if ( current_request_->parse( bytes_transferred ) )
        	{
                if ( !handle_request_header( current_request_ ) )
                {
                	trait_.event_close_after_response( *this, *current_request_ );
                	return;
                }

                if ( current_request_->state() != http::request_header::ok )
                {
                    trait_.error_request_parse_error( *this, *current_request_ );
                	return;
                }

                // handle_request_header() can switch to body decoding mode
                if ( body_read_call_back_.empty() )
                {
					// this consumes and decreases bytes_transferred
					current_request_.reset( new http::request_header( *current_request_, bytes_transferred,
							http::request_header::copy_trailing_buffer ) );
                }
                else
                {
                	const std::pair<char*, std::size_t> unparsed_read = current_request_->unparsed_buffer();
                	bytes_transferred = unparsed_read.second;
                	body_buffer_.resize( bytes_transferred );
                	std::copy( unparsed_read.first, unparsed_read.first + unparsed_read.second, body_buffer_.begin() );
                }
            }
        	else
        	{
        		// reading an incomplete header, all transfered bytes where consumed
        		bytes_transferred = 0;
        	}
        }

        issue_read( read_timeout_value() );
    }

    template < class Trait, class Connection, class Timer >
    void connection< Trait, Connection, Timer >::handle_keep_alive_timeout( const boost::system::error_code& ec )
    {
        if ( ec )
            return;

        trait_.event_keep_alive_timeout( *this );
        shutdown_close();
    }

    template < class Trait, class Connection, class Timer >
    bool connection< Trait, Connection, Timer >::handle_request_header(const boost::shared_ptr<const http::request_header>& new_request)
    {
        const boost::shared_ptr< async_response > response = trait_.create_response( this->shared_from_this(), new_request );
        assert( response.get() );

        responses_.push_back( response.get() );

        try
        {
        	trait_.event_before_response_started( *this, *new_request, *response );
            response->start();
        }
        catch (...)
        {
            responses_.pop_back();

            trait_.error_executing_request_handler( *this, *new_request, "error executing handler" );

            boost::shared_ptr<async_response> error_response(
                trait_.error_response(this->shared_from_this(), http::http_internal_server_error));

            if ( error_response.get() )
            {
                responses_.push_back(error_response.get());
                error_response->start();
            }

            shutdown_read();
            return false;
        }

        return true;
    }

    ///////////////////////
    // class blocked_write
	template < class Trait, class Connection, class Timer >
    template < class ConstBufferSequence, class WriteHandler >
    connection< Trait, Connection, Timer >::blocked_write<ConstBufferSequence, WriteHandler>::blocked_write(
        const ConstBufferSequence&  buffers,
        WriteHandler                handler)
        : buffers_(buffers)
        , handler_(handler)
    {
    }

	template < class Trait, class Connection, class Timer >
    template < class ConstBufferSequence, class WriteHandler >
    void connection< Trait, Connection, Timer >::blocked_write<ConstBufferSequence, WriteHandler>::operator()(Connection& con)
    {
        boost::asio::async_write(con, buffers_, handler_);
    }

	template < class Trait, class Connection, class Timer >
    template < class ConstBufferSequence, class WriteHandler >
    void connection< Trait, Connection, Timer >::blocked_write<ConstBufferSequence, WriteHandler>::cancel()
    {
        handler_(make_error_code(canceled_by_error), 0);
    }

    ////////////////////////////
    // class blocked_write_some
	template < class Trait, class Connection, class Timer >
    template < class ConstBufferSequence, class WriteHandler >
    connection< Trait, Connection, Timer >::blocked_write_some<ConstBufferSequence, WriteHandler>::blocked_write_some(
        const ConstBufferSequence&  buffers,
        WriteHandler                handler)
        : buffers_(buffers)
        , handler_(handler)
    {
    }

	template < class Trait, class Connection, class Timer >
    template < class ConstBufferSequence, class WriteHandler >
    void connection< Trait, Connection, Timer >::blocked_write_some<ConstBufferSequence, WriteHandler>::operator()(Connection& con)
    {
        con.async_write_some(buffers_, handler_);
    }

	template < class Trait, class Connection, class Timer >
    template < class ConstBufferSequence, class WriteHandler >
    void connection< Trait, Connection, Timer >::blocked_write_some<ConstBufferSequence, WriteHandler>::cancel()
    {
        handler_(make_error_code(canceled_by_error), 0);
    }

    template < class Connection, class Trait >
    boost::shared_ptr< connection< Trait, Connection > > create_connection(const Connection& con, Trait& trait)
    {
        const boost::shared_ptr< connection< Trait, Connection > > result(
            new connection< Trait, Connection >( con, trait ) );
        result->start();

        return result;
    }

} // namespace server

namespace boost {
namespace system {

template<> struct is_error_code_enum< server::error_codes >
{
    static const bool value = true;
};

}
}

#endif

