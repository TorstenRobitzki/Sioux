// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef SIOUX_SERVER_TEST_SESSION_GENERATOR_H_
#define SIOUX_SERVER_TEST_SESSION_GENERATOR_H_

#include "server/session_generator.h"

namespace server
{
	namespace test
	{
		/**
		 * @brief session_generator for testing, that generates session ids, by adding a consecutive number
		 *        to the name passed to the generator function.
		 */
		class session_generator : public server::session_generator
		{
		public:
			/**
			 * @brief constructs a new session_generator and initialize the internal consecutive number to zero.
			 */
			session_generator();

			/**
			 * @brief returns the passed network_connection_name concatenated with '/' and a consecutive number.
			 *
			 * So, a first call with "abc" for example will yield "abc/0", a second call with "bcd" will yield "bcd/1".
			 */
			virtual std::string operator()( const char* network_connection_name );
		private:
			// no copy; no assignment
			session_generator( const session_generator& );
			session_generator& operator=( const session_generator& );

			int	nr_;
		};
	}
}

#endif /* SIOUX_SERVER_TEST_SESSION_GENERATOR_H_ */
