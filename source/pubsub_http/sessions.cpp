#include "pubsub_http/sessions.h"
#include "server/session_generator.h"

namespace pubsub {
namespace http {

    //////////////
    // class session_impl
    class session_impl
    {
    public:
        explicit session_impl( const json::string& id ) : id_( id )
        {
        }

        const json::string& id() const
        {
            return id_;
        }

    private:
        json::string id_;
    };

    ////////////////////
    // class sessions
    sessions::sessions( server::session_generator& session_generator )
        : session_generator_( session_generator )
    {
    }

    sessions::~sessions()
    {
        for ( session_list_t::const_iterator s = sessions_.begin(); s != sessions_.end(); ++s )
            delete s->second;

        sessions_.clear();
    }

    session_impl* sessions::find_or_create_session( const json::string& session_id, const std::string& network_connection_name )
    {
        boost::mutex::scoped_lock lock( mutex_ );

        session_list_t::const_iterator pos = session_id.empty()
            ? sessions_.end()
            : sessions_.find( session_id );

        if ( pos == sessions_.end() )
        {
            const json::string new_id( session_generator_( network_connection_name ).c_str() );
            std::auto_ptr< session_impl > new_session( new session_impl( new_id ) );

            pos = sessions_.insert( std::make_pair( new_id, new_session.get() ) ).first;
            new_session.release();
        }

        return pos->second;
    }

    void sessions::idle_session( const session_impl* session )
    {
        boost::mutex::scoped_lock lock( mutex_ );

        session_list_t::const_iterator pos = sessions_.find( session->id() );
        assert( pos != sessions_.end() );
}


    ///////////////////
    // class session
    session::session( sessions& list, const json::string& session_id, const std::string& network_connection_name )
        : list_( list )
        , impl_( list.find_or_create_session( session_id, network_connection_name ) )
    {
        assert( impl_ );
    }

    session::~session()
    {
        list_.idle_session( impl_ );
    }

    const json::string& session::id() const
    {
        return impl_->id();
    }

}
}
