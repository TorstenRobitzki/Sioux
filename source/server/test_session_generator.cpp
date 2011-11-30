// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/test_session_generator.h"

#include "tools/asstring.h"

server::test::session_generator::session_generator()
	: nr_( 0 )
{
}

std::string server::test::session_generator::operator()( const char* network_connection_name )
{
	const std::string result = std::string( network_connection_name ) + "/" + tools::as_string( nr_ );
	++nr_;

	return result;
}

