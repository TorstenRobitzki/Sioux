// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef RACK_RUBY_TOOLS_H_
#define RACK_RUBY_TOOLS_H_

#include <ruby.h>
#include "tools/substring.h"

namespace rack
{
    int from_hash( VALUE hash, const char* entry );
    VALUE rb_str_new_sub( const tools::substring& s );
    tools::substring rb_str_to_sub( VALUE );
}

#endif /* RACK_RUBY_TOOLS_H_ */
