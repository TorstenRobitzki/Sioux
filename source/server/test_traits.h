// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SOURCE_SERVER_TEST_TRAIT_H
#define SIOUX_SOURCE_SERVER_TEST_TRAIT_H

#include "server/test_socket.h"
#include "server/test_response.h"
#include <vector>

namespace server {

    class async_response;

namespace test {

template <class Network = server::test::socket<const char*> >
class traits
{
public:
    traits() : pimpl_(new impl)
    {
    }

	typedef Network connection_type;

    template <class Connection>
    boost::weak_ptr<async_response> create_response(
        const boost::shared_ptr<Connection>&                    connection,
        const boost::shared_ptr<const server::request_header>&  header)
    {
        pimpl_->add_request(header);

        const boost::shared_ptr<response<Connection> > new_response(new response<Connection>(connection, header, "Hello"));
        new_response->start();            

        return boost::weak_ptr<async_response>(new_response);
    }

    std::vector<boost::shared_ptr<const server::request_header> > requests() const
    {
        return pimpl_->requests();
    }
private:
    class impl
    {
    public:
        void add_request(const boost::shared_ptr<const server::request_header>& r)
        {
            requests_.push_back(r);
        }

        std::vector<boost::shared_ptr<const server::request_header> > requests() const
        {
            return requests_;
        }
    private:
        std::vector<boost::shared_ptr<const server::request_header> >   requests_;
    };

    boost::shared_ptr<impl> pimpl_;
};

} // namespace test
} // namespace server 

#endif
