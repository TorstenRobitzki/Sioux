#include "server/transfer_buffer.h"

server::transfer_buffer_parse_error::transfer_buffer_parse_error(const std::string& error_text)
    : std::runtime_error(error_text)
{
}
