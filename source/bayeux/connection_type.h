// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef BAYEUX_CONNECTION_TYPE_H_
#define BAYEUX_CONNECTION_TYPE_H_

#include <iosfwd>

namespace bayeux
{
	/**
	 * @brief enumeration of the known (but not necessary supported) bayeux connection types
	 */
	enum connection_type
	{
		connection_long_poll,
		connection_callback_polling
	};

	/**
	 * @brief converts the given connection_type to a human readable
	 *
	 * connection_long_poll maps to long_poll, connection_callback_polling
	 * to callback_polling ect.
	 */
	std::ostream& operator<<( std::ostream& out, connection_type type );
}

#endif /* BAYEUX_CONNECTION_TYPE_H_ */
