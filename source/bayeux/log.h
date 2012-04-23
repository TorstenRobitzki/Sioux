// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_BAYEUX_LOG_H_
#define SIOUX_BAYEUX_LOG_H_

namespace bayeux
{
    struct empty_base;

    /**
     * @brief mixin to be mixed into the EventLog parameter of the connection_traits
     *
     * Example:
     * @begincode
     * typedef server::connection_traits< N, T, R, bayeux::stream_event_log< L > > trait_t;
     * @endcode
     */
    template < class Base = empty_base >
    class stream_event_log : public Base
    {
    public:
        template < class Parameter >
        explicit stream_event_log( const Parameter& param )
            : Base( param )
            , mutex_()
            , out_( param.logstream() )
        {
        }

        // uses std::cout as output stream
        stream_event_log();

        // must be defined to indicate, that this will implement the full bayeux logging interface
        typedef bool bayeux_logging_enabled;

        template < class Connection >
        void bayeux_start_response( const Connection& )
        {
            boost::mutex::scoped_lock lock( mutex_ );
            out_ << "bayeux_start_response..." << std::endl;
        }

        template < class Connection, class Payload >
        void bayeux_handle_requests( Connection&, const Payload& request_container )
        {
            boost::mutex::scoped_lock lock( mutex_ );
            out_ << "request: " << request_container << std::endl;
        }
    private:
        boost::mutex    mutex_;
        std::ostream&   out_;
    };
}

#endif /* SIOUX_BAYEUX_LOG_H_ */
