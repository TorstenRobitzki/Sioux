#include "pubsub/logging_adapter.h"
#include "pubsub/node.h"
#include "json/json.h"
#include <ostream>

namespace pubsub {

    logging_adapter::logging_adapter( adapter& a, std::ostream& l )
        : adapter_( a )
        , log_( l )
        , first_( true )
    {
        log_ << "[";
    }

    logging_adapter::~logging_adapter()
    {
        log_ << "]";
    }

    void logging_adapter::next_entry()
    {
        if ( !first_ )
            log_ << ",\n";

        first_ = false;
    }

    static const json::string call_token( "call" );
    static const json::string node_token( "node" );
    static const json::string result_token( "result" );

    namespace {

        template < class Base >
        class callback_base : public Base
        {
        public:
            callback_base( const json::string& function_name, const node_name& node, std::ostream& log, const boost::shared_ptr< Base >& cb )
                : protocol_()
                , log_( log )
                , cb_( cb )
                , result_logged_( false )
            {
                protocol_.add( call_token, function_name );
                protocol_.add( node_token, node.to_json() );
            }
        protected:
            template < class T >
            void result( const T& function_call_result )
            {
                protocol_.add( result_token, function_call_result );
                result();
            }

            void result()
            {
                if ( !result_logged_ )
                {
                    log_ << protocol_;
                    result_logged_ = true;
                }
            }

            boost::shared_ptr< Base > cb() const
            {
                return cb_;
            }

        private:
            json::object                    protocol_;
            std::ostream&                   log_;
            const boost::shared_ptr< Base > cb_;
            bool                            result_logged_;
        };
    }

    void logging_adapter::validate_node( const node_name& node, const boost::shared_ptr< validation_call_back >& cb )
    {
        static const json::string function_name( "validate_node" );

        class call_back : public callback_base< validation_call_back >
        {
        public:
            call_back( const node_name& node, std::ostream& log, const boost::shared_ptr< validation_call_back >& cb )
                : callback_base< validation_call_back >( function_name, node, log, cb )
            {
            }

        private:
            void is_valid()
            {
                result( json::true_val() );
                cb()->is_valid();
            }

            void not_valid()
            {
                result( json::false_val() );
                cb()->not_valid();
            }

            ~call_back()
            {
                result( json::false_val() );
            }
        };

        next_entry();
        adapter_.validate_node( node, boost::shared_ptr< validation_call_back >(
            static_cast< validation_call_back* >( new call_back( node, log_, cb ) ) ) );
    }

    void logging_adapter::authorize( const boost::shared_ptr<subscriber>& subscriber, const node_name& node, const boost::shared_ptr< authorization_call_back >& cb )
    {
        static const json::string function_name( "authorize" );

        class call_back : public callback_base< authorization_call_back >
        {
        public:
            call_back( const node_name& node, std::ostream& log, const boost::shared_ptr< authorization_call_back >& cb )
                : callback_base< authorization_call_back >( function_name, node, log, cb )
            {
            }

        private:
            void is_authorized()
            {
                result( json::true_val() );
                cb()->is_authorized();
            }

            void not_authorized()
            {
                result( json::false_val() );
                cb()->not_authorized();
            }

            ~call_back()
            {
                result( json::false_val() );
            }
        };

        next_entry();
        adapter_.authorize( subscriber, node, boost::shared_ptr< authorization_call_back >(
            static_cast< authorization_call_back* >( new call_back( node, log_, cb ) ) )  );
    }

    void logging_adapter::node_init( const node_name& node, const boost::shared_ptr< initialization_call_back >& cb )
    {
        static const json::string function_name( "node_init" );

        class call_back : public callback_base< initialization_call_back >
        {
        public:
            call_back( const node_name& node, std::ostream& log, const boost::shared_ptr< initialization_call_back >& cb )
                : callback_base< initialization_call_back >( function_name, node, log, cb )
            {
            }

        private:
            void initial_value( const json::value& value )
            {
                result( value );
                cb()->initial_value( value );
            }

            ~call_back()
            {
                result();
            }
        };

        next_entry();
        adapter_.node_init( node, boost::shared_ptr< initialization_call_back >(
            static_cast< initialization_call_back* >( new call_back( node, log_, cb ) ) ) );
    }

}
