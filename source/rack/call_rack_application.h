#ifndef RACK_CALL_RACK_APPLICATION_H
#define RACK_CALL_RACK_APPLICATION_H

#include <vector>
#include <ruby.h>
#include <boost/asio/ip/tcp.hpp>

namespace http {
    class request_header;
}


namespace rack
{
    class ruby_land_queue;

    /**
     * @brief Calls the applications run() function and transforms the result into a valid http response.
     *
     * If it was not possible, to create a response, the resulting vector is empty. If the application indicates
     * a shutdown request, the queues stop() function is called.
     *
     * @attention this function has to run on a ruby thread.
     */
    std::vector< char > call_rack_application( const std::vector< char >& body, const http::request_header& request,
        const boost::asio::ip::tcp::endpoint& endpoint, VALUE application, ruby_land_queue& queue );
}

#endif
