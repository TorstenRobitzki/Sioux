// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "bayeux/connection_type.h"

#include <ostream>


std::ostream& bayeux::operator<<( std::ostream& out, bayeux::connection_type type )
{
	switch ( type )
	{
	case bayeux::connection_long_poll:
		return out << "long_poll";
	case bayeux::connection_callback_polling:
		return out << "callback_polling";
	default:
		return out << "invalid value for bayeux::connection_type: " << static_cast< int >( type );
	}
}

