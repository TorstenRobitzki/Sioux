// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_TRANSFER_BUFFER_H
#define SIOUX_SOURCE_SERVER_TRANSFER_BUFFER_H

#include "http/message.h"
#include "http/parser.h"
#include <boost/asio/buffer.hpp>
#include <stdexcept>
#include <algorithm>
#include <string>

#ifdef min
#   undef min
#endif

namespace server 
{
    /**
     * @brief acception, that will be thrown, if protocol violation are detected by a transfer_buffer
     */
    class transfer_buffer_parse_error : public std::runtime_error
    {
    public:
        explicit transfer_buffer_parse_error(const std::string& error_text);
    };

    /**
     * @brief class that is responsible to buffer a body, while transfered between two connections
     *
     * Later this buffer can become responsible for translating different transfer encodings.
     *
     * As data written to a connection is to be read from the buffer and data read from a connection is 
     * written to the buffer, functions are named solely with the buffer in mind.
     *
     * The intended use is to have one asychron stream of reads from a orgin server, that writes data to the 
     * buffer and to have one stream of writes, writing the data to the client.
     *
     * It's not save to let multiple threads access functions of one object. But it's save to read  
     * data from a buffer passed by read_buffer() from one thread and to write data to the write_buffer() 
     * from an onther threads.
     */
    template <std::size_t BufferSize>
    class transfer_buffer
    {
    public:
        transfer_buffer() {}

        /**
         * @brief initialized this buffer for transfering a request or response body
         *
         * It's expected, that the passed buffer keeps staying valid over the live time
         * of the transfer_buffer. If there is unparsed data in the request, this data is
         * first handed out to a reader.
         */
        template <class Base>
        void start(http::message_base<Base>& request);

        /**
         * @brief returns the buffer, that can be read from the transfer_buffer
         *
         * If the read data was successfully passed around, the function data_read() have
         * to be called, to indicate, how much data was read.
         *
         * It's important to note, that the returned buffer, can have a size of 0. This
         * inidicates, that the buffer is currently empty.
         */
        boost::asio::const_buffers_1 read_buffer() const;

        /**
         * @brief returns the buffer, that can filled with data read from the net
         *
         * If writing to the passed buffer has been successfully done, data_written() have
         * to be called, to indicate, how much data was written to ths buffer.
         */
        boost::asio::mutable_buffers_1 write_buffer();

        /**
         * @brief returns true, if the body was completly transfered through this buffer
         */
        bool transmission_done() const;


        /**
         * @brief indicates, how much data was read from the buffer, that was passed by read_buffer()
         */
        void data_read(std::size_t);

        /** 
         * @brief indicates, how much data was written to the buffer, that was passed by write_buffer()
         */
        void data_written(std::size_t);

    private:
        transfer_buffer(const transfer_buffer&);
        transfer_buffer& operator=(const transfer_buffer&);

        char                            buffer_[BufferSize];
        std::pair<const char*, std::size_t>   unparsed_header_data_;
        std::size_t                     start_;     // start of the region, that is filled with data
        std::size_t                     end_;       // end of the region, that is filled with data

        std::size_t                     body_size_; // used, when state_ == size_given


        enum 
        {
            no_size_given,
            size_given,
            chunked,
            done
        }                               state_;

        // for chunked mode
        std::size_t                     current_chunk_;
        enum 
        {
            chunk_size_start,
            chunk_size,
            chunk_extension,
            chunk_size_lf,
            chunk_data,
            chunk_trailer_start, // any character but \r
            chunk_trailer,
            chunk_trailer_lf,
            chunk_last_trailer_lf
        }                               chunked_state_;


        void handle_chunked_write(std::size_t, const char* c);
    };

    /////////////////////////
    // implementation
    template <std::size_t BufferSize>
    template <class Base>
    void transfer_buffer<BufferSize>::start(http::message_base<Base>& header)
    {
        unparsed_header_data_ = header.unparsed_buffer();

        start_ = 0;
        end_   = 0;

        if ( header.option_available("Transfer-Encoding", "chunked") )
        {
            body_size_ = sizeof buffer_;
            chunked_state_ = chunk_size_start;
            state_ = chunked;

            handle_chunked_write(unparsed_header_data_.second, unparsed_header_data_.first);
        }
        else 
        {
            const http::header* const length_header = header.find_header("Content-Length");
            if (  length_header != 0 && http::parse_number(length_header->value().begin(), length_header->value().end(), body_size_) )
            {
                body_size_ -= std::min(body_size_, unparsed_header_data_.second);
                state_      = body_size_ != 0 ? size_given : done;
            }
            else
            {
                body_size_ = sizeof buffer_;
                state_ = no_size_given;
            }
        }
    }

    template <std::size_t BufferSize>
    boost::asio::const_buffers_1 transfer_buffer<BufferSize>::read_buffer() const
    {
        if ( unparsed_header_data_.second != 0 )
        {
            return boost::asio::buffer(unparsed_header_data_.first, unparsed_header_data_.second);
        }

        if ( start_ > end_ )
        {
            return boost::asio::buffer(&buffer_[start_], sizeof buffer_ - start_);
        }

        return boost::asio::buffer(&buffer_[start_], end_ - start_);
    }

    template <std::size_t BufferSize>
    boost::asio::mutable_buffers_1 transfer_buffer<BufferSize>::write_buffer()
    { 
        if ( state_ == done ) 
            return boost::asio::buffer(&buffer_[0], 0);

        if ( start_ <= end_ )
        {
            if ( end_ == sizeof buffer_ && start_ != 0 )
            {
                return boost::asio::buffer(&buffer_[0], std::min(start_-1, body_size_));
            }

            return boost::asio::buffer(&buffer_[end_], std::min(sizeof buffer_ - end_, body_size_));
        }

        return boost::asio::buffer(&buffer_[end_], std::min(start_ - end_ -1, body_size_));
    }

    template <std::size_t BufferSize>
    bool transfer_buffer<BufferSize>::transmission_done() const
    {
        return state_ == done && start_ == end_ && unparsed_header_data_.second == 0;
    }

    template <std::size_t BufferSize>
    void transfer_buffer<BufferSize>::data_read(std::size_t s)
    {
        if ( unparsed_header_data_.second != 0 )
        {
            assert(s <= unparsed_header_data_.second);
            unparsed_header_data_.second -= s;
            unparsed_header_data_.first  += s;

            return;
        }

        assert( s <= buffer_size(read_buffer()) );
        start_ += s;

        if ( start_ == sizeof buffer_ )
        {
            start_ = 0;

            if ( end_ == sizeof buffer_ )
                end_ = 0;
        }
    }

    template <std::size_t BufferSize>
    void transfer_buffer<BufferSize>::data_written(std::size_t s)
    {
        if ( end_ == sizeof buffer_ ) 
            end_ = 0;

        assert(end_ + s < start_ || end_ + s <= sizeof buffer_ );

        switch ( state_ )
        { 
        case no_size_given:
            if ( s == 0 )
                state_ = done;

            break;
        case size_given:
            assert(body_size_ >= s);
            body_size_ -= s;

            if ( body_size_ == 0 )
                state_ = done;

            break;
        case chunked:
            handle_chunked_write(s, &buffer_[end_]);
            break;
        default:
            assert(!"invalid state check transmission_done()");
        }

        end_ += s;

        if ( end_ == sizeof buffer_ && start_ != 0 )
            end_ = 0;
    }

    template <std::size_t BufferSize>
    void transfer_buffer<BufferSize>::handle_chunked_write(std::size_t size, const char* c)
    {
        for ( ; size && state_ != done; --size, ++c )
        {
            switch ( chunked_state_ )
            {
            case chunk_size_start:
                if ( !std::isxdigit(*c) )
                    throw transfer_buffer_parse_error("missing chunked size");

                current_chunk_ = http::xdigit_value(*c);
                chunked_state_ = chunk_size;
                break;
            case chunk_size:
                if ( std::isxdigit(*c) ) 
                {
                    if ( current_chunk_ > std::numeric_limits<std::size_t>::max() / 16 )
                        throw transfer_buffer_parse_error("chunk size to big");

                    current_chunk_ *= 16;
                    current_chunk_ +=  http::xdigit_value(*c);
                }
                else if ( *c == '\r' )
                {
                    chunked_state_ = chunk_size_lf;
                }
                else 
                {
                    chunked_state_ = chunk_extension;
                }

                break;
            case chunk_extension:
                if ( *c == '\r' )
                    chunked_state_ = chunk_size_lf;
                break;
            case chunk_size_lf:
                if ( *c != '\n' )
                    throw transfer_buffer_parse_error("missing linefeed in chunk size");

                chunked_state_ = current_chunk_ == 0 ? chunk_trailer_start : chunk_data;

                // add 2 for \r\n
                if ( current_chunk_ != 0 )
                    current_chunk_ += 2;
                break;
            case chunk_data:
                {
                    assert(current_chunk_);
                    size_t bite = std::min(size, current_chunk_);

                    current_chunk_ -= bite;
                    size           -= bite-1; // additional decrement in the for loop
                    c              += bite-1; // additional increment in the for loop

                    if ( current_chunk_ == 0 )
                        chunked_state_ = chunk_size_start;
                }
                break;
            case chunk_trailer_start:
                chunked_state_ = *c == '\r' ? chunk_last_trailer_lf : chunk_trailer;
                break;
            case chunk_trailer:
                if ( *c == '\r' ) 
                    chunked_state_ = chunk_trailer_lf;
                break;
            case chunk_trailer_lf:
                if ( *c != '\n' )
                    throw transfer_buffer_parse_error("missing linefeed in trailer");

                chunked_state_ = chunk_trailer_start;
                break;
            case chunk_last_trailer_lf:
                if ( *c != '\n' )
                    throw transfer_buffer_parse_error("missing linefeed in trailer");

                state_ = done;
                break;
            default:
                assert(!"invalid state");
            }
        }
    }

} // namespace server 

#endif // include guard

