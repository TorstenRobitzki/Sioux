#include "pubsub_http/sessions.h"
#include "server/session_generator.h"
#include "asio_mocks/test_timer.h"
#include <boost/bind.hpp>
#include <boost/asio/deadline_timer.hpp>

namespace pubsub {
namespace http {

    static const boost::posix_time::seconds session_timeout( 30 );

    //////////////
    // class session_impl
    class session_impl
    {
    public:
        explicit session_impl( const json::string& id )
            : id_( id )
            , in_use_( false )
        {
        }

        const json::string& id() const
        {
            return id_;
        }

        bool use()
        {
            const bool result = !in_use_;
            in_use_ = true;

            return result;
        }

        void unuse()
        {
            assert( in_use_ );
            in_use_ = false;
        }
        virtual ~session_impl() {}
    private:
        // const -> no synchronization required
        const json::string  id_;

        // will be synchronized by the lists mutex
        bool            in_use_;
    };

    ///////////////////////
    // class timed_session
    template < class TimeoutTimer >
    class timed_session : public session_impl
    {
    public:
        timed_session( const json::string& id, boost::asio::io_service& q )
            : session_impl( id )
            , timer_( q )
        {
        }

        TimeoutTimer timer_;
    };

    ////////////////////
    // class sessions
    template < class TimeoutTimer >
    sessions< TimeoutTimer >::sessions( server::session_generator& session_generator, boost::asio::io_service& queue )
        : queue_( queue )
        , session_generator_( session_generator )
    {
    }

    template < class TimeoutTimer >
    sessions< TimeoutTimer >::~sessions()
    {
        for ( session_list_t::const_iterator s = sessions_.begin(); s != sessions_.end(); ++s )
            delete s->second;

        sessions_.clear();
    }

    template < class TimeoutTimer >
    session_impl* sessions< TimeoutTimer >::find_or_create_session( const json::string& session_id, const std::string& network_connection_name )
    {
        boost::mutex::scoped_lock lock( mutex_ );

        session_list_t::const_iterator pos = session_id.empty()
            ? sessions_.end()
            : sessions_.find( session_id );

        if ( pos == sessions_.end() || !pos->second->use() )
        {
            const json::string new_id( session_generator_( network_connection_name ).c_str() );
            std::auto_ptr< session_impl > new_session( new timed_session< TimeoutTimer >( new_id, queue_ ) );

            pos = sessions_.insert( std::make_pair( new_id, new_session.get() ) ).first;
            new_session->use();
            new_session.release();
        }
        else
        {
            cancel_timer( pos->second );
        }

        return pos->second;
    }

    template < class TimeoutTimer >
    void sessions< TimeoutTimer >::idle_session( session_impl* session )
    {
        std::auto_ptr< session_impl > guarded_session( session );
        boost::mutex::scoped_lock lock( mutex_ );

        session_list_t::const_iterator pos = sessions_.find( session->id() );

        // if the session is removed from the list, it timed out before
        if ( pos != sessions_.end() )
        {
            session->unuse();
            guarded_session.release();

            setup_timeout( session );
        }
    }

    template < class TimeoutTimer >
    void sessions< TimeoutTimer >::shut_down()
    {
        boost::mutex::scoped_lock lock( mutex_ );
        for ( session_list_t::const_iterator s = sessions_.begin(); s != sessions_.end(); ++s )
        {
            cancel_timer( s->second );
        }
    }

    template < class TimeoutTimer >
    void sessions< TimeoutTimer >::setup_timeout( session_impl* s )
    {
        timed_session< TimeoutTimer >* const session = static_cast< timed_session< TimeoutTimer >* >( s );
        session->timer_.expires_from_now( session_timeout );
        session->timer_.async_wait( boost::bind( &sessions< TimeoutTimer >::timeout_session, this, s->id(), _1 ) );
    }

    template < class TimeoutTimer >
    void sessions< TimeoutTimer >::cancel_timer( session_impl* s )
    {
        timed_session< TimeoutTimer >* const session = static_cast< timed_session< TimeoutTimer >* >( s );
        boost::system::error_code ec;
        session->timer_.cancel( ec );
    }

    template < class TimeoutTimer >
    void sessions< TimeoutTimer >::timeout_session( const json::string& session_id, const boost::system::error_code& error )
    {
        if ( !error )
        {
            boost::mutex::scoped_lock lock( mutex_ );

            const session_list_t::iterator pos = sessions_.find( session_id );

            // if the session is now in use, it was used, just before the timeout callback gets executed
            if ( pos != sessions_.end() && pos->second->use() )
            {
                delete pos->second;
                sessions_.erase( pos );
            }
        }
    }

    ///////////////////
    // class session
    template < class Timer >
    session::session( sessions< Timer >& list, const json::string& session_id, const std::string& network_connection_name )
        : impl_( list.find_or_create_session( session_id, network_connection_name ) )
        , release_( boost::bind( &sessions< Timer >::idle_session, boost::ref( list ), impl_ ) )
    {
        assert( impl_ );
    }

    session::~session()
    {
        release_();
    }

    const json::string& session::id() const
    {
        return impl_->id();
    }

    template class sessions< boost::asio::deadline_timer >;
    template class sessions< asio_mocks::timer >;

    template session::session( sessions< boost::asio::deadline_timer >& list, const json::string&, const std::string& );
    template session::session( sessions< asio_mocks::timer >& list, const json::string&, const std::string& );
}
}
