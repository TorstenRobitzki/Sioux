// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#include "server/transfer_buffer.h"

server::transfer_buffer_parse_error::transfer_buffer_parse_error(const std::string& error_text)
    : std::runtime_error(error_text)
{
}