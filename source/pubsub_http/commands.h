#ifndef SIOUX_PUBSUB_HTTP_COMMANDS_H
#define SIOUX_PUBSUB_HTTP_COMMANDS_H

#include <boost/shared_ptr.hpp>

namespace json {
    class value;
    class object;
}

namespace pubsub {
    class subscriber;
    class root;
}

namespace pubsub {
namespace http {
    class session_reference;

namespace internal {

    class command
    {
    public:
        /**
         * @brief returns json::null() if the command can not be executed right now. Returns a coresponding json::object,
         *        if it was possible to execute or check the command
         */
        virtual json::value execute( const json::object& command, pubsub::root&, session_reference& ) const = 0;
        virtual ~command() {}
    };

    class subscribe : public command
    {
        virtual json::value execute( const json::object& command, pubsub::root&, session_reference& ) const;
    };

    class unsubscribe : public command
    {
        virtual json::value execute( const json::object& command, pubsub::root&, session_reference& ) const;
    };

}
}
}

#endif // include guard

