// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SOURCE_SIOUX_CONNECTION_H
#define SOURCE_SIOUX_CONNECTION_H

#include "http/request.h"
#include "http/http.h"
#include "server/response.h"
#include "tools/mem_tools.h"
#include <boost/asio/error.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <cassert>
#include <deque>
#include <map>
#include <algorithm>

/** @namespace server */
namespace server 
{
    class async_response;

    enum error_codes 
    {
        canceled_by_error
    };

    class connection_error_category : public boost::system::error_category 
    {
        virtual const char *     name() const;
        virtual std::string      message( int ev ) const;
    };

    boost::system::error_code make_error_code(error_codes e);

	/**
	 * @brief representation of physical connection from a client to this server
	 *
	 * It's the responsibility of the connection object to parse incomming requests 
	 * and to coordinate outgoing responses.
     *
     * The connection class is not responsible for http persisent connection support,
     * this have to be implemented within the responses.
	 */
    template <class Trait, class Connection = Trait::connection_type>
    class connection : public boost::enable_shared_from_this<connection<Trait, Connection> >
	{
	public:
        typedef Connection  socket_t;

        /**
         * @brief contructs a connection object by passing an IO object and trait object.
         */
        template <class ConArg>
		connection(ConArg Arg, const Trait& trait);

        ~connection();

        /**
         * @brief starts the first asynchron read on the connection passed to the c'tor
         */
        void start();

        /**
         * @brief interface for writing to the connection
         *
         * @pre a async_response implementation have to call response_started() once before calling async_write_some()
         * @post a async_response implementation have to call response_completed() once after calling async_write_some() the last time
         */
        template<
            typename ConstBufferSequence,
            typename WriteHandler>
        void async_write_some(
            const ConstBufferSequence&  buffers,
            WriteHandler                handler,
            async_response&             sender);

        /**
         * @brief to be called by an async_response to signal, that no more writes will be done
         *
         * Usualy this function will be called by a async_response implementations d'tor. This
         * have to be called exactly once by async_response that called response_started() before.
         */
        void response_completed(async_response& sender);

        /**
         * @brief end of response with error
         *
         * Same as response_completed(), but with an error code. The connection will do the nessary 
         * response to the client (if possible).
         *
         * In case of an http_internal_server_error error code, all running requests are canceled and the underlying 
         * connection will be closed.
         */
        void response_not_possible(async_response& sender, http::http_error_code = http::http_internal_server_error);

        /**
         * @brief to be called from a new response object, before any call to async_write_some()
         */
        void response_started(const boost::weak_ptr<async_response>& response);

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
	private:
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

        void issue_header_read();
		void handle_read(const boost::system::error_code& e, std::size_t bytes_transferred);

        /*
         * handles a newly read request_header and returns true, if further request_header should be 
         * read
         */
        bool handle_request_header(const boost::shared_ptr<const http::request_header>& new_request);

        // returns true, if the given response is the one, that currently is allowed to send
        bool is_current_response(async_response& sender) const;

        template <class Handler>
        void unblock_pending_writes(async_response& sender, const Handler&);

        // cancels all pending writes and reads, closes the connection
        void cancel_all();

        Connection                              connection_;
        Trait                                   trait_;

        boost::shared_ptr<http::request_header> current_request_;

        // as removing the responses is strictly required, the use of weak_ptr is not nessary,
        // but might be helpfull for debuging 
        typedef std::deque<boost::weak_ptr<async_response> > response_list;
        response_list                           responses_;

        static async_response& response_from_list(response_list::iterator i)
        {
            return *boost::shared_ptr<async_response>(*i);
        }

        static const async_response& response_from_list(response_list::const_iterator i)
        {
            return *boost::shared_ptr<const async_response>(*i);
        }


        typedef std::map<async_response*, std::vector<blocked_write_base*> >  blocked_write_list;
        blocked_write_list                      blocked_writes_;

        bool                                    current_response_is_sending_;
        bool                                    shutdown_read_;
 	};

    /**
     * @brief creates a new connection object and calls start() on it.
     * @relates connection
     */
	template <class Connection, class Trait>
    boost::shared_ptr<connection<Trait, Connection> > create_connection(const Connection& con, const Trait& trait);


    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // implementation
	template <class Trait, class Connection>
    template <class ConArg>
	connection<Trait, Connection>::connection(ConArg arg, const Trait& trait)
        : connection_(arg)
        , trait_(trait)
        , current_request_()
        , current_response_is_sending_(false)
        , shutdown_read_(false)
    {
    }

    template <class Trait, class Connection>
	connection<Trait, Connection>::~connection()
    {
        assert(blocked_writes_.empty());
    
        connection_.close();
    }

	template <class Trait, class Connection>
    void connection<Trait, Connection>::start()
    {
        current_request_.reset(new http::request_header);
        issue_header_read();
    }

	template <class Trait, class Connection>
    template<typename ConstBufferSequence, typename WriteHandler>
    void connection<Trait, Connection>::async_write_some(
        const ConstBufferSequence&  buffers,
        WriteHandler                handler,
        async_response&             sender)
    {
        if ( &response_from_list(responses_.begin()) == &sender )
        {
            connection_.async_write_some(buffers, handler);
            current_response_is_sending_ = true;
        }
        else
        {
            // hurry resposes that are blocking the sender
            for ( response_list::const_iterator r = responses_.begin(); ; ++r )
            {
                if ( !r->expired() )
                {
                    boost::shared_ptr<async_response> repo(*r);

                    if ( repo.get() == &sender )
                        break;

                    repo->hurry();
                }
            }

            // store send request until the current sender is ready
            blocked_write_list::mapped_type& writes = blocked_writes_[&sender];
            tools::save_push_back(new blocked_write<ConstBufferSequence, WriteHandler>(buffers, handler), writes);
        }
    }

    template <class Trait, class Connection>
    void connection<Trait, Connection>::response_completed(async_response& sender)
    {
        // there is no reason, why there should be outstanding, blocked writes from the current sender
        assert(blocked_writes_.erase(&sender) == 0);

        if ( &response_from_list(responses_.begin()) == &sender )
        {
            current_response_is_sending_ = false;
            responses_.pop_front();

            if ( responses_.empty() )
                return;

            const blocked_write_list::iterator pending_writes = blocked_writes_.find(&response_from_list(responses_.begin()));

            if ( pending_writes != blocked_writes_.end() )
            {
                // first remove all pending writes from the list
                blocked_write_list::mapped_type                             list;
                tools::ptr_container_guard<blocked_write_list::mapped_type> delete_blocked_writes(list);

                list.swap(pending_writes->second);
                blocked_writes_.erase(pending_writes);

                std::for_each(list.begin(), list.end(), 
                    boost::bind(&blocked_write_base::operator(), _1, boost::ref(connection_)));
            }
        }
    }

	template <class Trait, class Connection>
    void connection<Trait, Connection>::cancel_all()
    {

    }

	template <class Trait, class Connection>
    void connection<Trait, Connection>::response_not_possible(async_response& /*sender*/, http::http_error_code /*ec*/)
    {
#if 0
        const bool current_reponse = is_current_response(sender);
        boost::shared_ptr<async_response> error_response;

        // if no data was send from this response, consult the error-trait to get an error response
        if ( !current_reponse || !current_response_is_sending_ )
        {
            error_response = trait_.error_response(shared_from_this(), ec);
        }

        if ( error_response.get() )
        {
        }
        else
        {
            cancel_all();
        }
#endif 
    }

	template <class Trait, class Connection>
    void connection<Trait, Connection>::response_started(const boost::weak_ptr<async_response>& response)
    {
        responses_.push_back(response);
    }

    template <class Trait, class Connection>
    void connection<Trait, Connection>::shutdown_read()
    {
        if ( !shutdown_read_ )
        {
            shutdown_read_ = true;
            connection_.shutdown(boost::asio::ip::tcp::socket::shutdown_receive);
        }
    }

    template <class Trait, class Connection>
    void connection<Trait, Connection>::shutdown_close()
    {
        shutdown_read();
        connection_.shutdown(boost::asio::ip::tcp::socket::shutdown_send);
        connection_.close();
    }

    template <class Trait, class Connection>
    Connection& connection<Trait, Connection>::socket()
    {
        return connection_;
    }

	template <class Trait, class Connection>
    void connection<Trait, Connection>::issue_header_read()
    {
        const std::pair<char*, std::size_t> buffer = current_request_->read_buffer();
        assert(buffer.first && buffer.second);

        connection_.async_read_some(
            boost::asio::buffer(buffer.first, buffer.second),
            boost::bind(&connection::handle_read, 
                        boost::static_pointer_cast<connection<Trait, Connection> >(shared_from_this()),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred)
        );
    }

	template <class Trait, class Connection>
	void connection<Trait, Connection>::handle_read(const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        if (!error && bytes_transferred != 0)
        {
            while ( bytes_transferred != 0 && current_request_->parse(bytes_transferred) )
            {
                if ( !handle_request_header(current_request_) || current_request_->state() == http::request_header::buffer_full )
                    return;

                current_request_.reset(new http::request_header(*current_request_, bytes_transferred, http::request_header::copy_trailing_buffer));
            }

            issue_header_read();
        }
    }

	template <class Trait, class Connection>
    bool connection<Trait, Connection>::handle_request_header(const boost::shared_ptr<const http::request_header>& new_request)
    {
        trait_.create_response(shared_from_this(), new_request);

        for ( response_list::iterator response = responses_.begin(); response != responses_.end(); )
        {
            if ( response->expired() )
            {
                response = responses_.erase(response);
            }
            else 
            {
                ++response;
            }
        }

        if ( new_request->close_after_response() ) 
        {
            shutdown_read();
            return false;
        }

        return true;
    }

	template <class Trait, class Connection>
    template <class ConstBufferSequence, class WriteHandler>
    connection<Trait, Connection>::blocked_write<ConstBufferSequence, WriteHandler>::blocked_write(
        const ConstBufferSequence&  buffers,
        WriteHandler                handler)
        : buffers_(buffers)
        , handler_(handler)
    {
    }

	template <class Trait, class Connection>
    template <class ConstBufferSequence, class WriteHandler>
    void connection<Trait, Connection>::blocked_write<ConstBufferSequence, WriteHandler>::operator()(Connection& con)
    {
        con.async_write_some(buffers_, handler_);
    }

	template <class Trait, class Connection>
    template <class ConstBufferSequence, class WriteHandler>
    void connection<Trait, Connection>::blocked_write<ConstBufferSequence, WriteHandler>::cancel()
    {
        handler_(make_error_code(canceled_by_error), 0);
    }

    template <class Connection, class Trait>
    boost::shared_ptr<connection<Trait, Connection> > create_connection(const Connection& con, const Trait& trait)
    {
        const boost::shared_ptr<connection<Trait, Connection> > result(new connection<Trait, Connection>(con, trait));
        result->start();

        return result;
    }



} // namespace server

namespace boost {
namespace system {

template<> struct is_error_code_enum<server::error_codes>
{
    static const bool value = true;
};

}
}

#endif

