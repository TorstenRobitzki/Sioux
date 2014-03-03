#include "http/chunk_decoder.h"

http::chunk_decoder_parse_error::chunk_decoder_parse_error( const std::string& error_text )
    : std::runtime_error( error_text )
{
}
