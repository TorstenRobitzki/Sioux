namespace cluster
{
    class local_root_stream_interface
    {
    public:
        virtual ~local_root_stream_interface() {}
    };

    class local_root
    {
    public:
        local_root( pubsub::root_interface& local, const boost::shared_ptr< local_root_stream_interface >& connection );
    };
}
