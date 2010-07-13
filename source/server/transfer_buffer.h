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
    class transfer_buffer_parse_error : public std::runtime_error
    {
    public:
        explicit transfer_buffer_parse_error(const std::string& error_text);
    };

    /**
     * @brief class that is responsible to buffer a body, while transfered between two connections
     *
     * Later this buffer can become responsible for translating different transfer encodings
     *
     * As data written to a connection is to be read from the buffer and data read from a connection is 
     * written to the buffer, functions are named solely with the buffer in mind.
     */
    template <std::size_t BufferSize>
    class transfer_buffer
    {
    public:
        transfer_buffer() {}

        /**
         * @brief initialized this buffer for transfering a request or response body
         */
        template <class Base>
        void start(const http::message_base<Base>& request);

        boost::asio::const_buffers_1 read_buffer() const;
        boost::asio::mutable_buffers_1 write_buffer();

        /**
         * @brief returns true, if there is data to be read from the buffer
         */
        bool read_buffer_empty() const;

        /**
         * @brief returns true, if there is no room to write to the buffer
         */
        bool write_buffer_full() const;

        bool transmission_done() const;

        void data_read(std::size_t);
        void data_written(std::size_t);

    private:
        transfer_buffer(const transfer_buffer&);
        transfer_buffer& operator=(const transfer_buffer&);

        char            buffer_[BufferSize];
        std::size_t     start_;     // start of the region, that is filled with data
        std::size_t     end_;       // end of the region, that is filled with data

        std::size_t     body_size_; // used, when state_ == size_given

        enum 
        {
            no_size_given,
            size_given,
            chunked,
            done
        } state_;

        // for chunked mode
        std::size_t     current_chunk_;
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
        } chunked_state_;


        void handle_chunked_write(std::size_t);
    };

    /////////////////////////
    // implementation
    template <std::size_t BufferSize>
    template <class Base>
    void transfer_buffer<BufferSize>::start(const http::message_base<Base>& header)
    {
        start_ = 0;
        end_   = 0;

        if ( header.option_available("Transfer-Encoding", "chunked") )
        {
            body_size_ = sizeof buffer_;
            chunked_state_ = chunk_size_start;
            state_ = chunked;
        }
        else 
        {
            const http::header* const length_header = header.find_header("Content-Length");
            if (  length_header != 0 && http::parse_number(length_header->value().begin(), length_header->value().end(), body_size_) )
            {
                state_ = size_given;
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
    bool transfer_buffer<BufferSize>::read_buffer_empty() const
    {
        return start_ == end_;
    }

    template <std::size_t BufferSize>
    bool transfer_buffer<BufferSize>::write_buffer_full() const
    {
        return start_ == 0 && end_ == sizeof buffer_ 
            || end_ +1 == start_;
    }

    template <std::size_t BufferSize>
    bool transfer_buffer<BufferSize>::transmission_done() const
    {
        return state_ == done && read_buffer_empty();
    }

    template <std::size_t BufferSize>
    void transfer_buffer<BufferSize>::data_read(std::size_t s)
    {
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
            handle_chunked_write(s);
            break;
        default:
            assert(!"invalid state check transmission_done()");
        }

        end_ += s;

        if ( end_ == sizeof buffer_ && start_ != 0 )
            end_ = 0;
    }

    template <std::size_t BufferSize>
    void transfer_buffer<BufferSize>::handle_chunked_write(std::size_t size)
    {
        for ( std::size_t i = end_; size && state_ != done; --size, ++i )
        {
            switch ( chunked_state_ )
            {
            case chunk_size_start:
                if ( !std::isxdigit(buffer_[i]) )
                    throw transfer_buffer_parse_error("missing chunked size");

                current_chunk_ = http::xdigit_value(buffer_[i]);
                chunked_state_ = chunk_size;
                break;
            case chunk_size:
                if ( std::isxdigit(buffer_[i]) ) 
                {
                    if ( current_chunk_ > std::numeric_limits<std::size_t>::max() / 16 )
                        throw transfer_buffer_parse_error("chunk size to big");

                    current_chunk_ *= 16;
                    current_chunk_ +=  http::xdigit_value(buffer_[i]);
                }
                else if ( buffer_[i] == '\r' )
                {
                    chunked_state_ = chunk_size_lf;
                }
                else 
                {
                    chunked_state_ = chunk_extension;
                }

                break;
            case chunk_extension:
                if ( buffer_[i] == '\r' )
                    chunked_state_ = chunk_size_lf;
                break;
            case chunk_size_lf:
                if ( buffer_[i] != '\n' )
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
                    i           += bite-1; // additional increment in the for loop

                    if ( current_chunk_ == 0 )
                        chunked_state_ = chunk_size_start;
                }
                break;
            case chunk_trailer_start:
                chunked_state_ = buffer_[i] == '\r' ? chunk_last_trailer_lf : chunk_trailer;
                break;
            case chunk_trailer:
                if ( buffer_[i] == '\r' ) 
                    chunked_state_ = chunk_trailer_lf;
                break;
            case chunk_trailer_lf:
                if ( buffer_[i] != '\n' )
                    throw transfer_buffer_parse_error("missing linefeed in trailer");

                chunked_state_ = chunk_trailer_start;
                break;
            case chunk_last_trailer_lf:
                if ( buffer_[i] != '\n' )
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

