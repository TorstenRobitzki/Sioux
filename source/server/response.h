// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_RESPONSE_H
#define SIOUX_SOURCE_SERVER_RESPONSE_H

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>

namespace server
{
    /**
     * @brief The interface provides a mean to chain neighbours to change the direction they point to each other.
     * A response chain is provided by chain-links that point to each other by boost::shared_ptrs. Where the first
     * element is the connection itself and the remaining links are implemented by the response class.
     *
     * Every response_chain_link that is kept alive by having a shared_ptr pointing to itself by a pending IO
     * or such have to maintain a shared_ptr to the previous link. Where the previous link is the link that 
     * was created as the response to the request before. 
     *
     * Chain links that issued a write get an incremented reference counter from the chain_link where the write
     * was issued.
     */
    class response_chain_link : public boost::enable_shared_from_this<response_chain_link>
    {
    public:
        virtual ~response_chain_link() {}

        virtual void issue_async_write(const boost::asio::const_buffer&, const boost::shared_ptr<response_chain_link>& me) = 0;
        virtual void handle_async_write(const boost::system::error_code& error, std::size_t bytes_transferred, const boost::shared_ptr<response_chain_link>& me) = 0;
    };

    /**
     * @brief interface to something that will reponse to a given request. This interface is meant
     * to be the base class for responses that handle there IO asynchronous. None of the function to be implemented
     * should block.
     */
    class async_response : public response_chain_link
    {
    public:
        virtual ~async_response() {}

        /**
         * @brief issues IOs, that are needed to 
         * 
         * returns true, if IO was started successfully 
         */
        bool start();

    protected:
        // interface to derived classes

        /**
         * @brief constructs a async_response, with the next link in the chain.
         */
        explicit async_response(const boost::shared_ptr<response_chain_link>& next);

        /**
         * @brief returns a shared pointer to this. Just pass 'this' to let the function deduce the derived type
         * @attention this function can not be called from a c'tor.
         */
        template <class Self>
        boost::shared_ptr<Self> shared_from_this(Self* this_);

        /**
         * @brief returns a shared pointer to this. Just pass 'this' to let the function deduce the derived type
         * @attention this function can not be called from a c'tor.
         */
        template <class Self>
        boost::shared_ptr<const Self> shared_from_this(const Self* this_) const;

        /**
         * @brief Every response implementation have to report it's current state to the base class
         *
         * Possible states are:
         */
        enum state {
            /**
             * @brief are shared_ptr to this is stored in an IO queue
             */
            io_pending,

            /** 
             * @brief the response is not jet completely fulfilled. No shared_ptr is stored in an IO queue.
             */
            buffer_full,

            /**
             * @brief the response can not be fulfilled. No shared_ptr is stored in an IO queue.
             */
            error,

            /**
             * @brief the response is fulfilled. No shared_ptr is stored in an IO queue.
             */
            done
        };

        /** 
         * @brief start io, if nessary.
         * 
         * This function will be called right after contruction of the response object, by whom ever constructed
         * the object.
         */
        virtual state start_io() = 0;

        /**
         * @brief interface to deliver data to the connection.
         *
         * The passed current_state have to reflect the state, the object will be in, when it leaves it's IO
         * handler. There must not be more than one call to start_send_data(), before the end of the IO was 
         * reported by a invokation of end_send_data(). It's guarantied, that end_send_data() will not be
         * called from the call to start_send_data().
         */
        void start_send_data(state current_state, const char* data, std::size_t size);

        /**
         * @brief call back that will be call, when ever data was transmitted to the connection, or when 
         * an error occured.
         *
         * Implementations of this function should just issue new, needed IOs and shouldn't call start_send_data() 
         * directly from end_send_data().
         */
        virtual state end_send_data(const boost::system::error_code& error, std::size_t bytes_transferred) = 0;

        /**
         * @brief callback, that indicates, that a reponse next in the response chain is ready to send data
         *
         * The default implementation does nothing. The interface can be used to finish a long polling http
         * request.
         */
        virtual void next_response_waiting();

    private:
        // response_chain_link implementation
        void issue_async_write(const boost::asio::const_buffer&, const boost::shared_ptr<response_chain_link>& me);
        void handle_async_write(const boost::system::error_code& error, std::size_t bytes_transferred, const boost::shared_ptr<response_chain_link>& me);

        // if there was a write request stored from the prev_ link, it will be forwarded to the next_ link and 
        // next_ and prev_ will be reseted to remove this from the chain.
        void handle_pending();

        boost::shared_ptr<response_chain_link>  next_;
        boost::shared_ptr<response_chain_link>  prev_;

        // IO request from the prev_ link that must later be forwarded to the next_ link 
        boost::asio::const_buffer               pending_buffer_;
        bool                                    pending_;
        // all data send
        bool                                    done_;
    };


    template <class Self>
    boost::shared_ptr<Self> async_response::shared_from_this(Self* this_)
    {
        return boost::static_pointer_cast<Self>(boost::enable_shared_from_this<response_chain_link>::shared_from_this());
    }

    template <class Self>
    boost::shared_ptr<const Self> async_response::shared_from_this(const Self* this_) const
    {
        return boost::static_pointer_cast<Self>(boost::enable_shared_from_this<response_chain_link>::shared_from_this());
    }

} // namespace server

#endif

