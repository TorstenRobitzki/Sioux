// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SECURE_SESSION_GENERATOR_H_
#define SECURE_SESSION_GENERATOR_H_

#include "server/session_generator.h"
#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/cstdint.hpp>

namespace server
{
    /**
     * @brief class to generate randomly secure session id's
     */
    class secure_session_generator : public session_generator
    {
    private:
        virtual std::string operator()( const char* network_connection_name );

        boost::random::random_device                                        generator_;
        boost::random::uniform_int_distribution< boost::uint_least64_t >    distribution_;
    };
}

#endif /* SECURE_SESSION_GENERATOR_H_ */
