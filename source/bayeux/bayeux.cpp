// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "bayeux/bayeux.h"

#include <boost/bind.hpp>

#include "bayeux/session.h"
#include "asio_mocks/test_timer.h"
#include "tools/scope_guard.h"

namespace bayeux
{
    template < class Timer >
    connector< Timer >::connector( boost::asio::io_service& queue, pubsub::root& data,
	        server::session_generator& session_generator, const configuration& config )
		: queue_( queue )
	    , data_( data )
        , mutex_()
        , shutting_down_( false )
		, session_generator_( session_generator )
		, current_config_( new configuration( config ) )
		, sessions_()
		, index_()
	{
	}

    template < class Timer >
	session* connector< Timer >::find_session( const json::string& session_id )
	{
        boost::mutex::scoped_lock lock( mutex_ );

		const typename session_list_t::iterator pos = sessions_.find( session_id.to_std_string() );

		if ( pos != sessions_.end() )
		{
		    ++pos->second.use_count_;
		    return pos->second.session_.get();
		}

		return 0;
	}


    template < class Timer >
	session* connector< Timer >::handshake( const std::string& network_connection_name, const json::value* ext_value,
	    json::string& error_txt )
	{
        boost::mutex::scoped_lock lock( mutex_ );

        if ( shutting_down_ )
        {
            error_txt = json::string( "shutting down." );
            return 0;
        }

        std::string session_id = session_generator_( network_connection_name );

        for ( ; sessions_.find( session_id ) != sessions_.end(); session_id = session_generator_( network_connection_name ) )
            ;

        typename session_list_t::iterator pos;

        if ( users_actions_.get() )
        {
            boost::shared_ptr< session > session_obj;
            const std::pair< bool, json::string > hook_result =
                users_actions_->handshake( session_id, data_, current_config_, ext_value ? *ext_value : json::null(), session_obj );

            if ( !hook_result.first )
            {
                error_txt = hook_result.second;
                return 0;
            }

            pos = sessions_.insert( std::make_pair( session_id, session_data( session_obj, queue_ ) ) ).first;
        }
        else
        {
            const session_data data( session_id, data_, current_config_, queue_ );
            pos = sessions_.insert( std::make_pair( session_id, data ) ).first;
        }

        tools::scope_guard remove_session_if_index_fails =
            tools::make_obj_guard( *this, &connector< Timer >::remove_from_sessions, pos );

        session* const result = pos->second.session_.get();
        index_.insert( std::make_pair( result, pos ) );

        remove_session_if_index_fails.dismiss();

		return result;
	}

    template < class Timer >
    void connector< Timer >::idle_session( const session* session )
    {
        boost::mutex::scoped_lock lock( mutex_ );

        const typename session_index_t::iterator pos = index_.find( session );
        assert( pos != index_.end() );
        assert( pos->second->second.use_count_ > 0 );

        if ( --pos->second->second.use_count_ == 0 )
        {
            Timer& timer = *pos->second->second.timer_;
            timer.expires_from_now( current_config_->session_timeout() );
            timer.async_wait(
                boost::bind( &connector< Timer >::session_timeout_reached, boost::ref( *this ), session, _1 ) );
        }
    }

    template < class Timer >
    void connector< Timer >::drop_session( const json::string& session_id )
    {
        boost::mutex::scoped_lock lock( mutex_ );

        const typename session_list_t::iterator pos = sessions_.find( session_id.to_std_string() );

        if ( pos != sessions_.end() && pos->second.use_count_ == 0 )
        {
            const typename session_index_t::size_type delete_index_elements =
                index_.erase( pos->second.session_.get() );

            static_cast< void >( delete_index_elements );
            assert( delete_index_elements == 1 );

            sessions_.erase( pos );
        }

        assert( sessions_.size() == index_.size() );
    }

    template < class Timer >
    std::pair< bool, json::string > connector< Timer >::publish( const json::string& channel, const json::value& data,
        const json::object& message, session& s  )
    {
        if ( users_actions_.get() )
        {
            return users_actions_->publish( channel, data, message, s, data_ );
        }
        else
        {
            static const std::pair< bool, json::string > error_result( false, json::string( "no handler installed" ) );
            return error_result;
        }
    }

    template < class Timer >
    boost::asio::io_service& connector< Timer >::queue()
    {
        return queue_;
    }

    template < class Timer >
    void connector< Timer >::shut_down()
    {
        boost::mutex::scoped_lock lock( mutex_ );

        shutting_down_ = true;

        for ( typename session_list_t::iterator s = sessions_.begin(); s != sessions_.end(); ++s )
            s->second.shut_down();
    }

    template < class Timer >
    void connector< Timer >::remove_from_sessions( typename session_list_t::iterator pos )
    {
        sessions_.erase( pos );
    }

    template < class Timer >
    void connector< Timer >::session_timeout_reached( const session* s, const boost::system::error_code& ec )
    {
        if ( ec )
            return;

        boost::shared_ptr< session > session_to_be_deleted;

        {
            boost::mutex::scoped_lock lock( mutex_ );

            const typename session_index_t::iterator pos = index_.find( s );

            if ( pos != index_.end() && pos->second->second.use_count_ == 0 )
            {
                session_to_be_deleted = pos->second->second.session_;
                sessions_.erase( pos->second );
                index_.erase( pos );

                assert( sessions_.size() == index_.size() );
            }
        }

        if ( session_to_be_deleted.get() )
            session_to_be_deleted->close();
    }

    template < class Timer >
    connector< Timer >::session_data::session_data( const std::string& session_id, pubsub::root& data,
        const boost::shared_ptr< const configuration >& config, boost::asio::io_service& queue )
        : use_count_( 1u )
        , remove_( false )
        , session_( new session( session_id, data, config ) )
        , timer_( new Timer ( queue ) )
    {
    }

    template < class Timer >
    connector< Timer >::session_data::session_data( const boost::shared_ptr< session >& session,
        boost::asio::io_service& queue )
        : use_count_( 1u )
        , remove_( false )
        , session_( session )
        , timer_( new Timer ( queue ) )
    {
    }

    template < class Timer >
    void connector< Timer >::session_data::shut_down()
    {
        boost::system::error_code ec;
        timer_->cancel( ec );
        session_->shut_down();
    }

    template class connector<>;
    template class connector< asio_mocks::timer >;
}

