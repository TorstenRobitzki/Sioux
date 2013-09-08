#include "pubsub_http/connector.h"
#include "asio_mocks/test_timer.h"

namespace pubsub {
namespace http {

template < class Timer >
connector< Timer >::connector( boost::asio::io_service& queue, pubsub::root& data )
{
}

template class connector<>;
template class connector< asio_mocks::timer >;

}
}
