#include "pubsub/root.h"
#include <boost/function.hpp>

namespace cluster
{

    class remote_root_stream_interface
    {
    public:
        virtual ~remote_root_stream_interface() {}
    };

    /**
     * @brief class that implements a stub for a remote root_interface implementation
     */
    class remote_root : public pubsub::root_interface
    {
    public:
        template < class Archive
        remote_root( remote_failed_interface& failure_interface, stream& transport );
    private:
        virtual void subscribe( const boost::shared_ptr< pubsub::subscriber >&, const pubsub::node_name& node_name);
        virtual bool unsubscribe( const boost::shared_ptr< pubsub::subscriber >&, const pubsub::node_name& node_name);
        virtual unsigned unsubscribe_all( const boost::shared_ptr< pubsub::subscriber >& );
        virtual void update_node( const pubsub::node_name& node_name, const json::value& new_data );
    };

    /**
     * @brief implements the root_interface by using a remote_root and a fallback for that remote_root
     *
     * As long as the remote_root is working correctly.
     */
    class fallback_remote_root : public pubsub::root_interface, private remote_failed_interface
    {
    public:
        fallback_remote_root( stream& transport&, const boost::shared_ptr< pubsub::root_interface >& fallback );
    };
}
