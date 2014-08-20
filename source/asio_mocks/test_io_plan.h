#ifndef SIOUX_SRC_SERVER_ASIO_MOCKS_IO_PLAN_H
#define SIOUX_SRC_SERVER_ASIO_MOCKS_IO_PLAN_H

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/system/error_code.hpp>
#include <utility>
#include <vector>

namespace asio_mocks
{
    struct read;

    /**
     * @brief plan for simulated reads from a socket, with the data to be read and delays between them
     */
    class read_plan
    {
    public:
        /**
         * @brief an empty plan
         */
        read_plan();

        /**
         * @brief implizit conversion from read to read_plan
         */
        read_plan( const read& );

        typedef std::pair< std::string, boost::posix_time::time_duration > item;

        /**
         * @brief returns the data for the next, read to perform. The time duration is the time until the read
         * should be simulated.
         * @pre the plan must not be empty
         */
        item next_read();

        /**
         * @brief if the last item doesn't contain data to be read, it will be added to the last item.
         * If the last item contains already data, a new item is added
         */
        void add(const std::string&);

        /**
         * @brief adds a new item, with no data and the given delay
         */
        void delay(const boost::posix_time::time_duration& delay);

        /**
         * @brief a function object to be executed.
         * The function will be executed when the item that next will be added will be pulled out with a call to next_read().
         * So if a function is added with execute( f ) and than, one item is added with add( "foo" ), one item can
         * be pulled with next_read() and will next_read() get executed, f will be executed.
         */
        void execute( const boost::function< void() >& f );

        /**
         * @brief returns true, if the list is empty
         */
        bool empty() const;

    private:
        struct impl;
        boost::shared_ptr< impl >	pimpl_;
    };

    /**
     * @brief defines what read is next to be simulated
     * @sa read_plan
     */
    struct read
    {
        explicit read(const std::string& s);
        explicit read(const char* s);

        template <class Iter>
        read(Iter begin, Iter end) : data(begin, end) {}

        read(const char* begin, const char* end) : data(begin, end) {}
        read(char* begin, char* end) : data(begin, end) {}

        std::string data;
    };

    /**
     * @brief simulates a read of zero bytes and thus a grateful disconnect
     * @sa read_plan
     */
    struct disconnect_read
    {
    };

    /**
     * @brief simulates a delay in the delivery of data while reading from a network
     * @sa read_plan
     */
    struct delay
    {
        explicit delay(const boost::posix_time::time_duration&);
        boost::posix_time::time_duration delay_value;
    };

    /**
     * @brief adds a read to the read_plan
     * @relates read_plan
     */
    read_plan operator<<(read_plan plan, const read&);

    /**
     * @brief adds a delay to the read_plan
     * @relates read_plan
     */
    read_plan operator<<(read_plan plan, const delay&);

    /**
     * @brief adds a read of zero bytes and thus a grateful disconnect to the the given read_plan
     */
    read_plan operator<<( read_plan plan, const disconnect_read& );

    /**
     * @brief adds the given function to be execute before the next read is simulated
     */
    read_plan operator<<( read_plan plan, const boost::function< void() >& );

    /**
     * @brief plan, that describes, how large issued writes are performed and how much
     * delay it to be issued between writes
     */
    class write_plan
    {
    public:
        /**
         * @brief an empty plan
         */
        write_plan();

        struct item
        {
            std::size_t                         size;
            boost::posix_time::time_duration    delay;
            boost::system::error_code           error_code;
        };

        /**
         * @brief returns the data for the next, write to perform. The time duration is the time until the write
         * should be simulated.
         * @pre the plan must not be empty
         */
        item next_write();

        /**
         * @brief if the last item doesn't contain data to be write, it will be added to the last item.
         * If the last item contains already a size, a new item is added
         */
        void add(const std::size_t&);

        /**
         * @brief adds a new item, with no data and the given delay
         */
        void delay(const boost::posix_time::time_duration& delay);

        /**
         * @brief adds a new item, with the error_code field set to ec
         */
        void error( const boost::system::error_code& ec );

        /**
         * @brief returns true, if the list is empty
         */
        bool empty() const;

    private:
        struct impl;
        boost::shared_ptr< impl > 	pimpl_;
    };

    /**
     * @brief simulates the consumption of a given number of bytes by the network
     * @sa write_plan
     */
    struct write
    {
        explicit write(std::size_t s);
        std::size_t size;
    };

    /**
     * @brief adds a write to the write_plan
     * @relates write_plan
     */
    write_plan operator<<(write_plan plan, const write&);

    /**
     * @brief adds a delay to the write_plan
     * @relates write_plan
     */
    write_plan operator<<(write_plan plan, const delay&);

    /**
     * @brief adds a error the the write_plan
     * @relates write_plan
     */
    write_plan operator<<( write_plan plan, const boost::system::error_code& ec );

} // namespace asio_mocks

#endif
