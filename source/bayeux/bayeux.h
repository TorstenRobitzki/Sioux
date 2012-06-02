// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_BAYEUX_BAYEUX_H_
#define SIOUX_BAYEUX_BAYEUX_H_

#include <boost/asio/deadline_timer.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <utility>

#include "bayeux/response.h"
#include "bayeux/adapter.h"
#include "bayeux/configuration.h"
#include "pubsub/root.h"
#include "server/connection.h"
#include "json/json.h"

namespace server {
	class async_response;
	class session_generator;
}

namespace http {
	class request_header;
}

/** @namespace bayeux */
namespace bayeux
{
	class session;

    /**
	 * @brief Class that creates responses to request targeting a bayeux server component
	 *
	 * The template is currently explicit instantiated for boost::asio::deadline_timer and server::test::timer
	 */
	template < class Timer = boost::asio::deadline_timer >
	class connector
	{
	public:
	    /**
	     * @brief this is the main interface to the bayeux component
	     *
	     * Construct a connector and response to http request that are meant to be bayeux requests
	     *
	     * @param queue a queue, that is actively used to perform IO and timeouts
	     * @param data the pubsub data root to store and retrieve data
	     * @param session_generator a crypto-graphically random generator. The access to the session generator
	     *        might by from different threads, but synchronized t one thread at a time.
	     * @param config configuration values for the bayeux protocol implementation
	     */
		connector( boost::asio::io_service& queue, pubsub::root& data, server::session_generator& session_generator,
		    const configuration& config );

		/**
		 * @brief construct a connector with a user adapter to hook in handshake and publish messages.
		 *
		 * The user_acctions object must stay volid over the entire live time of the connector. If the connector
		 * is used with multiple threads, the adapter might get called from multiple threads at the same time.
		 */
		template < class SessionData >
		connector( boost::asio::io_service& queue, pubsub::root& data, server::session_generator& session_generator,
            adapter< SessionData >& user_acctions, const configuration& config );

		/**
		 * @brief creates a new response object for a given http request.
		 */
		template < class Connection >
		boost::shared_ptr< server::async_response > create_response(
			const boost::shared_ptr< Connection >&                    connection,
			const boost::shared_ptr< const http::request_header >&    header );

		/**
		 * @brief looks up the session with the given session_id. If no such session exists, a null pointer
		 *        is returned.
		 *
		 * If the session isn't used anymore, it has to be returned by caling idle_session().
		 */
		session* find_session( const json::string& session_id );

		/**
		 * @brief this function will create a new session with the given network_connection_name
         *
		 * If the session isn't used anymore, it has to be returned by calling idle_session().
		 *
		 * @return the returned pointer will be null, if handshaking fails and error_txt will be set to
		 *         an error message by the users handshake hook.
		 */
		session* handshake( const std::string& network_connection_name, const json::value* ext_value, json::string& error_txt );

		/**
		 * @brief this session is currently idle
		 *
		 * This function has to be called after a session was obtained by a call to find_session() or
		 * handshake(), isn't used anymore. The session timeout starts expiring after a call to idle_session() and
		 * will stop expiring with a next call to find_session()
		 */
		void idle_session( const session* session );

        /**
         * @brief removes the session with the given id
         *
         * If no such session is given, no special action is taken.
         */
        void drop_session( const json::string& session_id );

        /**
         * @brief forwards the information of a publish message to a user adapter
         *
         * If the constructor that takes an adapter<> was used to construct this instance, the call is forwared to that
         * adapter. If no adapter is in use, the function returns std::make_pair( false, "no handler installed" )
         */
        std::pair< bool, json::string > publish( const json::string& channel, const json::value& data,
            const json::object& message, session& s );

        /**
         * @brief reference to the queue, that have to be used by connections for timer ect.
         */
        boost::asio::io_service& queue();
	private:
        boost::asio::io_service&                    queue_;
		pubsub::root& 				                data_;

        boost::mutex                                mutex_;
		server::session_generator&	                session_generator_;
		boost::shared_ptr< const configuration >    current_config_;

		struct session_data
		{
		    unsigned                            use_count_;
		    bool                                remove_;
		    boost::shared_ptr< session >        session_;
		    boost::shared_ptr< Timer >          timer_;

		    session_data( const std::string& session_id, pubsub::root& data,
                const boost::shared_ptr< const configuration >& config, boost::asio::io_service& queue );

		    explicit session_data( const boost::shared_ptr< session >& session, boost::asio::io_service& queue );
		};

		typedef std::map< std::string, session_data > session_list_t;
		session_list_t				                sessions_;

		typedef std::map< const session*, typename session_list_t::iterator > session_index_t;
		session_index_t                             index_;

		struct handle_user_actions
		{
	        virtual std::pair< bool, json::string > handshake( const std::string& session_id, pubsub::root& data,
                const boost::shared_ptr< const configuration >& config, const json::value& ext,
                boost::shared_ptr< session >& new_session  ) = 0;

	        virtual std::pair< bool, json::string > publish( const json::string& channel, const json::value& data,
	            const json::object& message, session& client, pubsub::root& root ) = 0;

		    virtual ~handle_user_actions() {}
		};

		const boost::scoped_ptr< handle_user_actions >  users_actions_;

		template < class SessionData >
		static handle_user_actions* create_user_handler( adapter< SessionData >& user_acctions );

		void session_timeout_reached( const session*, const boost::system::error_code& ec );

		void remove_from_sessions( typename session_list_t::iterator );
		void remove_session( const session* );
	};


	// implementation
    template < class Timer >
	template < class Connection >
	boost::shared_ptr< server::async_response > connector< Timer >::create_response(
		const boost::shared_ptr< Connection >&                    connection,
		const boost::shared_ptr< const http::request_header >&    header )
	{
		return boost::shared_ptr< server::async_response >( new response< Connection >( connection, header, *this ) );
	}

    template < class Timer >
    template < class SessionData >
    connector< Timer >::connector( boost::asio::io_service& queue, pubsub::root& data,
        server::session_generator& session_generator, adapter< SessionData >& user_acctions,
        const configuration& config )
        : queue_( queue )
        , data_( data )
        , mutex_()
        , session_generator_( session_generator )
        , current_config_( new configuration( config ) )
        , sessions_()
        , index_()
        , users_actions_( create_user_handler( user_acctions ) )
    {
    }

    template < class Timer >
    template < class SessionData >
    typename connector< Timer >::handle_user_actions* connector< Timer >::create_user_handler(
        adapter< SessionData >& user_acctions )
    {
        struct session_with_user_data : session
        {
            SessionData user_data_;

            session_with_user_data( const std::string& session_id, pubsub::root& root,
                        const boost::shared_ptr< const configuration >& config, const SessionData& user_data )
                : session( session_id, root, config )
                , user_data_( user_data )
            {
            }
        };

        struct typed_user_actions : handle_user_actions
        {
            explicit typed_user_actions( adapter< SessionData >& user_acctions )
                : hooks_( user_acctions )
            {
            }

            std::pair< bool, json::string > handshake( const std::string& session_id, pubsub::root& root,
                const boost::shared_ptr< const configuration >& config, const json::value& ext,
                boost::shared_ptr< session >& new_session )
            {
                SessionData session_data = SessionData(); // the type SessionData have to have a default c'tor
                const std::pair< bool, json::string > result = hooks_.handshake( ext, session_data );

                if ( result.first )
                {
                    new_session.reset( static_cast< session* >(
                        new session_with_user_data( session_id, root, config, session_data ) ) );
                }

                return result;
            }

            std::pair< bool, json::string > publish( const json::string& channel, const json::value& data,
                const json::object& message, session& client, pubsub::root& root )
            {
                return hooks_.publish( channel, data, message, static_cast< session_with_user_data& >( client ).user_data_, root );
            }

            adapter< SessionData >& hooks_;
        };

        return new typed_user_actions( user_acctions );
    }
}

#endif /* SIOUX_BAYEUX_BAYEUX_H_ */
