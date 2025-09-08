// Copyright 2025 Fancapital Inc.  All rights reserved.
#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <memory>

#include "base_define.h"
#include "mdc_client_factory.h"
#include "client_interface.h"
#include "message_handle.h"
#include "feeder/feeder.h"

using namespace com::htsc::mdc::gateway;
using namespace com::htsc::mdc::model;
using namespace com::htsc::mdc::insight::model;

namespace co {
    int64_t BsFlag2Std(const int32_t& bs_flag);
    int64_t OrderType2Std(const int32_t& order_type);
    int64_t Status2Std(const string& status);
    int64_t Market2Std(ESecurityIDSource source);
    int64_t CpFlag2Std(const std::string& value);
    int8_t Type2Std(ESecurityType security_type);
}  // namespace co
