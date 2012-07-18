// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "tools/exception_handler.h"
#include <stdexcept>
#include <sstream>

namespace tools
{
#   define TOOLS_CATCH_STD_EXCEPTION( e, o ) \
    catch ( const std::e& exc ) { \
        o << "std::"#e" : \"" << exc.what() << "\""; \
        throw; \
    }

#   define TOOLS_CATCH_EXCEPTION( e, o, name ) \
    catch ( const e& exc ) { \
        o << name " exception : \"" << exc << "\""; \
        throw; \
    }

    void exception_text( std::ostream& output, rethrow_options to_be_rethrown )
    {
        try
        {
            try
            {
                throw;
            }

            TOOLS_CATCH_STD_EXCEPTION( range_error, output )
            TOOLS_CATCH_STD_EXCEPTION( overflow_error, output )
            TOOLS_CATCH_STD_EXCEPTION( underflow_error, output )
            TOOLS_CATCH_STD_EXCEPTION( runtime_error, output )

            TOOLS_CATCH_STD_EXCEPTION( domain_error, output )
            TOOLS_CATCH_STD_EXCEPTION( invalid_argument, output )
            TOOLS_CATCH_STD_EXCEPTION( length_error, output )
            TOOLS_CATCH_STD_EXCEPTION( out_of_range, output )
            TOOLS_CATCH_STD_EXCEPTION( logic_error, output )

            TOOLS_CATCH_STD_EXCEPTION( exception, output )

            TOOLS_CATCH_EXCEPTION( int, output, "integer" )
            TOOLS_CATCH_EXCEPTION( char*, output, "c string" )
            TOOLS_CATCH_EXCEPTION( std::string, output, "std string" )

            catch ( ... )
            {
                output << "unknown exception";
                throw;
            }

        }
        catch ( ... )
        {
            if ( to_be_rethrown == rethrow )
                throw;
        }
    }

#   undef TOOLS_CATCH_STD_EXCEPTION
#   undef TOOLS_CATCH_EXCEPTION

    std::string exception_text()
    {
        std::ostringstream out;
        exception_text( out, do_not_rethrow );

        return out.str();
    }

}
