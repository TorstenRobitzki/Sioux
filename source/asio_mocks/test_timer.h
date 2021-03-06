#ifndef SIOUX_SOURCE_SERVER_TEST_TIMER_H
#define SIOUX_SOURCE_SERVER_TEST_TIMER_H

#include <boost/asio/io_service.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>

namespace asio_mocks {
    /**
     * @brief test replacement for a boost::asio::deadline_timer
     */
    class timer : boost::noncopyable
    {
    public:
        explicit timer( boost::asio::io_service& );

        ~timer();

        typedef boost::posix_time::ptime time_type;
        typedef boost::posix_time::time_duration duration_type;

        template< typename WaitHandler >
        void async_wait( WaitHandler handler );

        /**
         * @brief Cancel any asynchronous operations that are waiting on the timer.
         *
         * This function forces the completion of any pending asynchronous wait operations against the timer.
         * The handler for each cancelled operation will be invoked with the boost::asio::error::operation_aborted
         * error code. Cancelling the timer does not change the expiry time.
         */
        std::size_t cancel();
        std::size_t cancel( boost::system::error_code & ec );

        /**
         * @brief Get the timer's expire time as an absolute time.
         */
        time_type expires_at() const;

        /**
         * @brief Set the timer's expire time as an absolute time.
         *
         * This function sets the expiry time. Any pending asynchronous wait operations will be cancelled.
         * The handler for each cancelled operation will be invoked with the boost::asio::error::operation_aborted
         * error code.
         */
        std::size_t expires_at( const time_type & expiry_time );

        /**
         * @brief overload version the function above.
         * @param expiry_time sets a new expiration time for the timer
         * @param ec will never by altered in the test library.
         */
        std::size_t expires_at( const time_type & expiry_time, boost::system::error_code & ec );

        /**
         * @brief Get the timer's expire time relative to now.
         */
        duration_type expires_from_now() const;

        /**
         * @brief Set the timer's expire time relative to now.
         */
        std::size_t expires_from_now( const duration_type & expiry_time );
        std::size_t expires_from_now( const duration_type & expiry_time, boost::system::error_code & ec );
    private:
        void async_wait_impl( const boost::function< void ( const boost::system::error_code& ) >& handler );

        boost::asio::io_service& queue_;
    };

    /**
     * @brief returns the currently simulated time
     *
     * If the simulated time was not initialized (by calling current_time( ptime ) ), the function will return
     * boost::posix_time::time_from_string( "1970-01-01 00:00:00")
     */
    boost::posix_time::ptime current_time();

    /**
     * @brief sets the currently simulated time
     * @pre new_time >= current_time()
     * @post current_time() will yield new_time
     */
    void current_time( const boost::posix_time::ptime& new_time );

    /**
     * @brief resets the time to 1970-01-01 00:00:00
     */
    void reset_time();

    /**
     * @brief updates the current time
     *
     * Equivalent to current_time( current_time() + delay )
     * @pre delay >= 0
     */
    void advance_time( const boost::posix_time::time_duration& delay );

    /**
     * @brief advances the simulated time up to the next point, where a timer is scheduled to expire.
     *
     * @return returns the number of scheduled timer that reached there expiration times by advancing the
     *         time to the nearest expirtion time.
     */
    unsigned advance_time();

    // implementation
    template< typename WaitHandler >
    void timer::async_wait( WaitHandler handler )
    {
        async_wait_impl( handler );
    }

} // namespace asio_mocks

#endif 


