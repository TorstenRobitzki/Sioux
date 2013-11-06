#include "pubsub_http/sessions.h"
#include "pubsub_http/response.h"
#include "pubsub/node.h"
#include "pubsub/pubsub.h"
#include "pubsub/root.h"
#include "server/session_generator.h"
#include "asio_mocks/test_timer.h"
#include <boost/bind.hpp>
#include <boost/asio/deadline_timer.hpp>

namespace pubsub {
namespace http {

    static const boost::posix_time::seconds session_timeout( 20 );

    //////////////
    // class session_impl
    class session_impl : public subscriber
    {
    public:
        session_impl( const json::string& id )
            : id_( id )
            , use_counter_( 0 )
        {
        }

        const json::string& id() const
        {
            return id_;
        }

        // Use this session without associating an update_interface
        void use()
        {
            ++use_counter_;
        }

        // stop using this session without prior associating an update_interface
        bool unuse()
        {
            assert( use_counter_ );
            --use_counter_;

            return use_counter_ == 0;
        }

        void wait( const boost::shared_ptr< waiting_connection >& connection )
        {
            boost::mutex::scoped_lock lock( mutex_ );

            if ( connection_.get() )
                connection_->second_connection();

            connection_ = connection;
        }

        bool wake_up( const boost::shared_ptr< waiting_connection >& connection )
        {
            boost::mutex::scoped_lock lock( mutex_ );

            const bool result = connection_ == connection;
            connection_.reset();

            return result;
        }

        bool not_in_use() const
        {
            return use_counter_ == 0;
        }


    private:
        void on_event( const json::object& update, const json::object& respond )
        {
            boost::shared_ptr< waiting_connection > con;
            json::array updates;
            json::array responds;

            {
                boost::mutex::scoped_lock lock( mutex_ );

                if ( !update.empty() )
                    updates_.add( update );

                if ( !respond.empty() )
                    responds_.add( respond );

                if ( connection_.get() )
                {
                    con.swap( connection_ );
                    updates.swap( updates_ );
                    responds.swap( responds_ );
                }
            }

            if ( con.get() )
                con->update( responds, updates );
        }

        virtual void on_update( const pubsub::node_name& name, const node& data )
        {
            json::object update;
            update.add( internal::key_token, name.to_json() );
            update.add( internal::data_token, data.data() );
            update.add( internal::version_token, data.current_version().to_json() );

            on_event( update, json::object() );
        }

        virtual void on_invalid_node_subscription(const node_name& node)
        {
            std::cout << "on_invalid_node_subscription: " << node << std::endl;
        }

        virtual void on_unauthorized_node_subscription(const node_name& node)
        {
            std::cout << "on_unauthorized_node_subscription: " << node << std::endl;
        }

        virtual void on_failed_node_subscription(const node_name& node)
        {
            std::cout << "on_failed_node_subscription: " << node << std::endl;
        }

        // const -> no synchronization required
        const json::string                      id_;

        // this members are synchronized by the sessions list.
        unsigned                                use_counter_;

        boost::mutex                            mutex_;
        boost::shared_ptr< waiting_connection > connection_;
        json::array                             updates_;
        json::array                             responds_;
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
    sessions< TimeoutTimer >::sessions( server::session_generator& session_generator, boost::asio::io_service& queue, pubsub::root& root )
        : queue_( queue )
        , root_( root )
        , session_generator_( session_generator )
    {
    }

    template < class TimeoutTimer >
    sessions< TimeoutTimer >::~sessions()
    {
    }

    template < class TimeoutTimer >
    std::pair< session_impl*, bool > sessions< TimeoutTimer >::find_or_create_session( const json::string& session_id, const std::string& network_connection_name )
    {
        boost::mutex::scoped_lock lock( mutex_ );

        session_list_t::const_iterator pos = session_id.empty()
            ? sessions_.end()
            : sessions_.find( session_id );

        const bool created = pos == sessions_.end();

        if ( created )
        {
            const json::string new_id( session_generator_( network_connection_name ).c_str() );
            boost::shared_ptr< session_impl > new_session( new timed_session< TimeoutTimer >( new_id, queue_ ) );

            pos = sessions_.insert( std::make_pair( new_id, new_session) ).first;
        }
        else
        {
            cancel_timer( *pos->second );
        }

        pos->second->use();
        return std::make_pair( pos->second.get(), created );
    }

    template < class TimeoutTimer >
    void sessions< TimeoutTimer >::idle_session( session_impl* session )
    {
        boost::mutex::scoped_lock lock( mutex_ );

        session_list_t::const_iterator pos = sessions_.find( session->id() );
        assert( pos != sessions_.end() );

        if ( session->unuse() )
            setup_timeout( *session );
    }

    template < class TimeoutTimer >
    void sessions< TimeoutTimer >::wait_for_session_event( session_impl* session, const boost::shared_ptr< waiting_connection >& connection )
    {
        assert( session );
        assert( connection.get() );
        boost::mutex::scoped_lock lock( mutex_ );

        session_list_t::const_iterator pos = sessions_.find( session->id() );
        assert( pos != sessions_.end() );

        pos->second->wait( connection );
    }

    template < class TimeoutTimer >
    bool sessions< TimeoutTimer >::wake_up( session_impl* session, const boost::shared_ptr< waiting_connection >& connection  )
    {
        assert( session );
        assert( connection.get() );
        boost::mutex::scoped_lock lock( mutex_ );

        session_list_t::const_iterator pos = sessions_.find( session->id() );
        assert( pos != sessions_.end() );

        return pos->second->wake_up( connection );
    }

    template < class TimeoutTimer >
    void sessions< TimeoutTimer >::subscribe( session_impl* session, const node_name& node_name )
    {
        assert( session );
        boost::mutex::scoped_lock lock( mutex_ );

        session_list_t::const_iterator pos = sessions_.find( session->id() );
        assert( pos != sessions_.end() );

        root_.subscribe( pos->second, node_name );
    }

    template < class TimeoutTimer >
    void sessions< TimeoutTimer >::unsubscribe( session_impl* session, const node_name& node_name )
    {
        assert( session );
        boost::mutex::scoped_lock lock( mutex_ );

        session_list_t::const_iterator pos = sessions_.find( session->id() );
        assert( pos != sessions_.end() );

        root_.unsubscribe( pos->second, node_name );
    }

    template < class TimeoutTimer >
    void sessions< TimeoutTimer >::shut_down()
    {
        boost::mutex::scoped_lock lock( mutex_ );
        for ( session_list_t::const_iterator s = sessions_.begin(); s != sessions_.end(); ++s )
        {
            cancel_timer( *s->second );
        }
    }

    template < class TimeoutTimer >
    void sessions< TimeoutTimer >::setup_timeout( session_impl& s )
    {
        timed_session< TimeoutTimer >& session = static_cast< timed_session< TimeoutTimer >& >( s );
        session.timer_.expires_from_now( session_timeout );
        session.timer_.async_wait( boost::bind( &sessions< TimeoutTimer >::timeout_session, this, s.id(), _1 ) );
    }

    template < class TimeoutTimer >
    void sessions< TimeoutTimer >::cancel_timer( session_impl& s )
    {
        timed_session< TimeoutTimer >& session = static_cast< timed_session< TimeoutTimer >& >( s );
        boost::system::error_code ec;
        session.timer_.cancel( ec );
    }

    template < class TimeoutTimer >
    void sessions< TimeoutTimer >::timeout_session( const json::string& session_id, const boost::system::error_code& error )
    {
        if ( !error )
        {
            boost::mutex::scoped_lock lock( mutex_ );

            const session_list_t::iterator pos = sessions_.find( session_id );

            // if the session_reference is now in use, it was used, just before the timeout callback gets executed
            if ( pos != sessions_.end() && pos->second->not_in_use() )
            {
                root_.unsubscribe_all( pos->second );
                sessions_.erase( pos );
            }
        }
    }

    json::string id( session_impl* session )
    {
        assert( session );
        return session->id();
    }

    template class sessions< boost::asio::deadline_timer >;
    template class sessions< asio_mocks::timer >;
}
}
