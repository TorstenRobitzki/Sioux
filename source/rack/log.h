// Copyright (c) Torrox GmbH & Co KG. All rights reserved.
// Please note that the content of this file is confidential or protected by law.
// Any unauthorised copying or unauthorised distribution of the information contained herein is prohibited.

#ifndef RACK_LOG_H_
#define RACK_LOG_H_

#include "tools/log.h"

namespace rack
{
    class log_context_t : public logging::context {};

    extern log_context_t log_context;
}
#endif /* RACK_LOG_H_ */
