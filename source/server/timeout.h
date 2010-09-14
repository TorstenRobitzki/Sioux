// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_TIMEOUT_H
#define SIOUX_SOURCE_SERVER_TIMEOUT_H

#include "server/error_code.h"
#include <boost/asio/error.hpp>

namespace server 
{
    /**
     * @brief function, that implements read with timeout
     */
    template<
        typename AsyncReadStream,
        typename MutableBufferSequence,
        typename ReadHandler>
    void async_read_some_with_to(
        AsyncReadStream&                        stream,
        const MutableBufferSequence&            buffers,
        ReadHandler                             handler,
        boost::asio::deadline_timer&            timer,
        const boost::posix_time::time_duration& time_out);

    /**
     * @brief function, that implements writing with timeout
     */
    template<
        typename AsyncReadStream,
        typename ConstBufferSequence,
        typename WriteHandler>
    void async_write_some_with_to(
        AsyncReadStream&                        stream,
        const ConstBufferSequence&              buffers,
        WriteHandler                            handler,
        boost::asio::deadline_timer&            timer,
        const boost::posix_time::time_duration& time_out);

    /**
     * @brief function, that implements writing with timeout
     */
    template<
        typename AsyncReadStream,
        typename ConstBufferSequence,
        typename WriteHandler>
    void async_write_with_to(
        AsyncReadStream&                        stream,
        const ConstBufferSequence&              buffers,
        WriteHandler                            handler,
        boost::asio::deadline_timer&            timer,
        const boost::posix_time::time_duration& time_out);

    // implementations
    namespace details {

        template<
            typename AsyncReadStream,
            typename MutableBufferSequence,
            typename ReadHandler>
        struct async_read_some_with_to_t
        {
            AsyncReadStream&                socket;
            MutableBufferSequence           buffers;   
            ReadHandler                     handler;
            boost::asio::deadline_timer&    timer;

            void operator()(const boost::system::error_code& error)
            {
                if ( !error )
                {
                    boost::system::error_code ec;
                    socket.close(ec);
                }
            }

            void operator()(const boost::system::error_code& error, std::size_t bytes_transferred)
            {
                boost::system::error_code ec;
                timer.cancel(ec);

                if ( error == boost::asio::error::operation_aborted )
                {
                    handler(make_error_code(server::time_out), bytes_transferred);
                }
                else
                {
                    handler(error, bytes_transferred);
                }
            }

        };

        template<
            typename AsyncWriteStream,
            typename ConstBufferSequence,
            typename WriteHandler>
        struct async_write_some_with_to_t
        {
            AsyncWriteStream&               socket;
            ConstBufferSequence             buffers;   
            WriteHandler                    handler;
            boost::asio::deadline_timer&    timer;

            void operator()(const boost::system::error_code& error)
            {
                if ( !error )
                {
                    boost::system::error_code ec;
                    socket.close(ec);
                }
            }

            void operator()(const boost::system::error_code& error, std::size_t bytes_transferred)
            {
                boost::system::error_code ec;
                timer.cancel(ec);

                if ( error == boost::asio::error::operation_aborted )
                {
                    handler(make_error_code(server::time_out), bytes_transferred);
                }
                else
                {
                    handler(error, bytes_transferred);
                }
            }
        };
    }

    template<
        typename AsyncReadStream,
        typename MutableBufferSequence,
        typename ReadHandler>
    void async_read_some_with_to(
        AsyncReadStream&                        stream,
        const MutableBufferSequence&            buffers,
        ReadHandler                             handler,
        boost::asio::deadline_timer&            timer,
        const boost::posix_time::time_duration& time_out)
    {
        details::async_read_some_with_to_t<AsyncReadStream, MutableBufferSequence, ReadHandler> timeout_handler = 
            {stream, buffers, handler, timer};

        timer.expires_from_now(time_out);
        timer.async_wait(timeout_handler);

        stream.async_read_some(buffers, timeout_handler);
    }

    template<
        typename AsyncReadStream,
        typename ConstBufferSequence,
        typename WriteHandler>
    void async_write_some_with_to(
        AsyncReadStream&                        stream,
        const ConstBufferSequence&              buffers,
        WriteHandler                            handler,
        boost::asio::deadline_timer&            timer,
        const boost::posix_time::time_duration& time_out)
    
    {
        async_write_some_with_to_t<AsyncReadStream, ConstBufferSequence, WriteHandler> timeout_handler = 
            {stream, buffers, handler, timer};

        timer.expires_from_now(time_out);
        timer.async_wait(timeout_handler);

        stream.async_write_some(buffers, timeout_handler);
    }

    template<
        typename AsyncReadStream,
        typename ConstBufferSequence,
        typename WriteHandler>
    void async_write_with_to(
        AsyncReadStream&                        stream,
        const ConstBufferSequence&              buffers,
        WriteHandler                            handler,
        boost::asio::deadline_timer&            timer,
        const boost::posix_time::time_duration& time_out)
    {
        details::async_write_some_with_to_t<AsyncReadStream, ConstBufferSequence, WriteHandler> timeout_handler = 
            {stream, buffers, handler, timer};

        timer.expires_from_now(time_out);
        timer.async_wait(timeout_handler);

        boost::asio::async_write(stream, buffers, timeout_handler);
    }
}

#endif

