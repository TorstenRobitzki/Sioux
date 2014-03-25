namespace pubsub {
    class root_interface;
}

namespace cluster
{
    /**
     * @brief creates root_interface implementations based on configurations
     *
     */
    class root_factory
    {
    public:
        boost::shared_ptr< pubsub::root_interface > create_root(type, config) const;
    };
}
