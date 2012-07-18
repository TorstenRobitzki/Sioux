// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef TOOLS_EXCEPTION_HANDLER_H_
#define TOOLS_EXCEPTION_HANDLER_H_

#include <iosfwd>

namespace tools
{
    enum rethrow_options { rethrow, do_not_rethrow };

    /**
     * @brief extracts a error text from an exception, that is currently thrown
     * @pre there must be an exception currently catched
     */
    void exception_text( std::ostream& output, rethrow_options to_be_rethrown );

    /**
     * @brief extracts a text from the currently catched exception
     */
    std::string exception_text();
}

#endif /* TOOLS_EXCEPTION_HANDLER_H_ */
