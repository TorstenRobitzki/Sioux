// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef RACK_RUBY_LAND_QUEUE_H_
#define RACK_RUBY_LAND_QUEUE_H_

#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <deque>
#include <vector>

namespace http {
    class request_header;
}

namespace rack
{
    /**
     * @brief interface to the rack application
     */
    class application_interface
    {
    public:
        /**
         * the implementation have to construct the environment hash, call the rack application and transform
         * the result into a buffer to be transmitted to the client. If no response is intended, the function
         * returns an empty vector.
         */
        virtual std::vector< char > call( const std::vector< char >& body, const http::request_header& request,
            const boost::asio::ip::tcp::endpoint& ) = 0;

        virtual ~application_interface() {}
    };

    /**
     * @brief queue used to send upcalls to the ruby thread that called Rack::Handler::Sioux.run()
     *
     * This is some kind of workaround as there seems to be no reliable way to call a ruby callback.
     * (using rb_thread_call_with_gvl() results in undefined symbols on OSX with 1.9.2)
     */
    class ruby_land_queue
    {
    public:
        ruby_land_queue();

        typedef boost::function< void ( application_interface& ) > call_back_t;

        /**
         * @brief this function can be called without having the gvl (global vm lock) at hand
         *
         * @return true, if the callback was stored for latter executing
         */
        bool push( const call_back_t& );

        /**
         * @brief indicates that the process should shut down
         */
        void stop();

        /**
         * @brief this function blocks and releases the gvl while waiting for new work to come
         *
         * If new work is processed, the glv is reacquired
         * @attention it's expected that this function is only called by threads, that are created by ruby.
         */
        void process_request( application_interface& application );

        // internal
        void wait( boost::unique_lock<boost::mutex>& lock );
    private:

        boost::mutex        mutex_;
        boost::condition    condition_;
        bool                stop_flag_;
        typedef std::deque< call_back_t > queue_t;
        queue_t             queue_;
    };
}

#endif /* RACK_RUBY_LAND_QUEUE_H_ */
