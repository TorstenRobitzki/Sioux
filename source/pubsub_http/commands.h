#ifndef SIOUX_PUBSUB_HTTP_COMMANDS_H
#define SIOUX_PUBSUB_HTTP_COMMANDS_H

namespace json {
    class value;
    class object;
}

namespace pubsub {
namespace http {
namespace internal {

    class command
    {
    public:
        /**
         * @brief returns json::null() if the command can not be executed right now. Returns a coresponding json::object,
         *        if it was possible to execute or check the command
         */
        virtual json::value execute( const json::object& command ) const = 0;
        virtual ~command() {}
    };

    class subscribe : public command
    {
        virtual json::value execute( const json::object& command ) const;
    };

    class unsubscribe : public command
    {
        virtual json::value execute( const json::object& command ) const;
    };

}
}
}

#endif // include guard

