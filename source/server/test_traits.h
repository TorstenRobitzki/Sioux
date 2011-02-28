// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_TEST_TRAIT_H
#define SIOUX_SOURCE_SERVER_TEST_TRAIT_H

#include "server/test_socket.h"
#include "server/test_response.h"
#include "server/error.h"
#include "server/traits.h"
#include <vector>

namespace server {

    class async_response;

namespace test {

	class proxy_connector;

template <class Connection>
struct response_factory
{
    response_factory() {}

    template <class T>
    explicit response_factory(const T&) {}

    template <class Trait>
    static boost::shared_ptr<async_response> create_response(
        const boost::shared_ptr<Connection>&                    connection,
        const boost::shared_ptr<const http::request_header>&    header,
              Trait&)
    {
        const boost::shared_ptr<response<Connection> > new_response(new response<Connection>(connection, header, "Hello"));
        return boost::shared_ptr<async_response>(new_response);
    }
};

/**
 * @brief traits class for testing.
 *
 * The default behaviour of an incomming request is to answer with a simple "Hello" string
 */
template <class Network = server::test::socket<const char*>, 
          template <class> class  ResponseFactory = response_factory>
class traits : public server::connection_traits<Network, ResponseFactory<Network> >
{
public:
    traits() : pimpl_(new impl(0, 0))
    {
    }

    traits(proxy_connector& p, boost::asio::io_service& io) : pimpl_(new impl(&p, &io))
    {
    }

	typedef Network connection_type;

    template <class Connection>
    boost::shared_ptr<async_response> create_response(
        const boost::shared_ptr<Connection>&                    connection,
        const boost::shared_ptr<const http::request_header>&    header)
    {
        pimpl_->add_request(header);
        const boost::shared_ptr<async_response> result(ResponseFactory<Connection>::create_response(connection, header, *this));
        pimpl_->add_response(result);

        return result;
    }

    std::vector<boost::shared_ptr<const http::request_header> > requests() const
    {
        return pimpl_->requests();
    }

    template <class Connection>
    boost::shared_ptr<async_response> error_response(const boost::shared_ptr<Connection>& con, http::http_error_code ec) const
    {
        boost::shared_ptr<async_response> result(new server::error_response<Connection>(con, ec));
        return result;
    }

    proxy_connector& proxy() const
    {
        return pimpl_->proxy();
    }

    boost::asio::io_service& io_queue() const
    {
        return pimpl_->io_queue();
    }

    std::vector<boost::shared_ptr<server::async_response> > responses() const
    {
        return pimpl_->responses();
    }

    void reset_responses()
    {
        pimpl_->reset_responses();
    }

private:
    class impl
    {
    public:
        explicit impl(proxy_connector* p, boost::asio::io_service* i) 
            : requests_()
            , proxy_(p)
            , io_(i)
        {
        }

        void add_request(const boost::shared_ptr<const http::request_header>& r)
        {
            requests_.push_back(r);
        }

        void add_response(const boost::shared_ptr<server::async_response>& r)
        {
            responses_.push_back(r);
        }

        std::vector<boost::shared_ptr<const http::request_header> > requests() const
        {
            return requests_;
        }

        std::vector<boost::shared_ptr<server::async_response> > responses() const
        {
            return responses_;
        }

        void reset_responses()
        {
            responses_.clear();
        }

        proxy_connector& proxy() const
        {
            assert(proxy_);
            return *proxy_;
        }

        boost::asio::io_service& io_queue() const
        {        
            assert(io_);
            return *io_;
        }

    private:
        std::vector<boost::shared_ptr<const http::request_header> >     requests_;
        std::vector<boost::shared_ptr<server::async_response> >         responses_;
        proxy_connector*                                                proxy_;
        boost::asio::io_service*                                        io_;
    };

    boost::shared_ptr<impl> pimpl_;
};

} // namespace test
} // namespace server 

#endif
