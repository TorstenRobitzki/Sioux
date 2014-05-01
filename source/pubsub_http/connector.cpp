#include "pubsub_http/connector.h"
#include "asio_mocks/test_timer.h"

namespace pubsub {
namespace http {

template < class Timer >
connector< Timer >::connector( boost::asio::io_service& queue, pubsub::root& data )
    : default_session_generator_()
    , session_list_( default_session_generator_, queue, data )
    , data_( data )
{
}

template < class Timer >
connector< Timer >::connector( boost::asio::io_service& queue, pubsub::root& data, server::session_generator& session_id_generator )
    : default_session_generator_()
    , session_list_( session_id_generator, queue, data )
    , data_( data )
{
}

template < class Timer >
connector< Timer >::~connector()
{
    session_list_.shut_down();
}

template class connector<>;
template class connector< asio_mocks::timer >;

}
}
