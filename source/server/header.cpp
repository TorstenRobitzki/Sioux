// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/header.h"

namespace server {

    //////////////////
    // struct header
    header::header(const tools::substring& name, const tools::substring& value)
        : name_(name)
        , value_(value)
    {
    }
    
    header::header()
        : name_()
        , value_()
    {
    }

    tools::substring header::name() const
    {
        return name_;
    }

    tools::substring header::value() const
    {
        return value_;
    }

} // namespace server



