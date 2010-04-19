// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include <boost/shared_ptr.hpp>
#include <string>

namespace test_client 
{

/* internal interfaces */
class command_impl;

/**
 * a test client 
 */
class client
{
public:
    /**
     * @brief a new client that connects to local host on port 80
     */
    client();

    /**
     * @brief a new client that connects to the given host and port 80
     */
    explicit client(const std::string& host);

    /**
     * @brief a new client that connects to the given host and port
     */
    client(const std::string& host, unsigned short port);

    ~client();

    /**
     * @brief sends the given data to the connection
     */
    void send(const char* data, std::size_t length);

    /**
     * @brief closes an established connection 
     */
    void disconnect();

private:
    class impl;
    boost::shared_ptr<impl> pimpl_;
};

/**
 * @brief base class for a command executed by a client
 */
class command
{
public:
    void execute(client&);
    ~command();
protected:
    explicit command(command_impl*);
private:
    boost::share_ptr<command_impl>  pimpl_;
};

class command_list
{
public:
    command_list();

    typedef 
private:
    boost::shared_ptr<std::vector<command> > list_;
};

class client_list
{
public:
    /**
     * @brief empty list of clients
     */
    client_list();

    ~client_list();

    /** 
     * @brief executes the stored commands
     */
    void execute();
private:
    class impl;
    boost::shared_ptr<impl> pimpl_;
};

} // test_client