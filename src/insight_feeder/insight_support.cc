// Copyright 2025 Fancapital Inc.  All rights reserved.
#include "insight_support.h"

namespace co {
int64_t BsFlag2Std(const int32_t& bs_flag) {
    int64_t std_bs_flag = 0;
    switch (bs_flag) {
        case 1:
        case 3:
            std_bs_flag = kBsFlagBuy;
            break;
        case 2:
        case 4:
            std_bs_flag = kBsFlagSell;
            break;
        default:
        // LOG_ERROR << "unknown insight_bs_flag: " << bs_flag;
            break;
    }
    return std_bs_flag;
}

int64_t OrderType2Std(const int32_t& order_type) {
    int64_t std_order_type = 0;
    switch (order_type) {
        case 1:
            std_order_type = kQOrderTypeMarket;
            break;
        case 2:
            std_order_type = kQOrderTypeLimit;
            break;
        case 3:
            std_order_type = kQOrderTypeSelfMarket;
            break;
        case 10:
            std_order_type = kQOrderTypeWithdraw;
            break;
        default:
            // LOG_ERROR << "unknown insight_order_type: " << _order_type;
            break;
    }
    return std_order_type;
}

int64_t Status2Std(const string& status) {
    int64_t std_state = kStateOK;
    if (!status.empty()) {
        char c = status[0];
        if (c == '8') {
            std_state = kStateSuspension;
        } else if (c == '9') {
            std_state = kStateBreak;
        }
    }
    return std_state;
}

int64_t Market2Std(ESecurityIDSource source) {
    int64_t market = 0;
    switch (source) {
        case ESecurityIDSource::XSHE:
            market = kMarketSZ;
            break;
        case XSHG:
            market = kMarketSH;
            break;
        case CCFX:
            market = kMarketCFFEX;
            break;
        case XSGE:
            market = kMarketSHFE;
            break;
        case INE:
            market = kMarketINE;
            break;
        case XDCE:
            market = kMarketDCE;
            break;
        case XZCE:
            market = kMarketCZCE;
            break;
        case XHKG:
            market = kMarketHK;
            break;
        case HKSC:
            market = kMarketHK;
            break;
        case CSI:
            market = kMarketCSI;
            break;
        case CNI:
            market = kMarketCNI;
            break;
        case XBSE:
            market = kMarketBJ;
            break;
        default:
            // LOG_ERROR << "not valid source: " << source;
            break;
    }
    return market;
}

int64_t CpFlag2Std(const std::string& value) {
    int64_t cp_flag = 0;
    if (!value.empty()) {
        char c = value[0];
        if (c == 'C') {
            cp_flag = kCpFlagCall;
        } else if (c == 'P') {
            cp_flag = kCpFlagPut;
        }
    }
    return cp_flag;
}

int8_t Type2Std(com::htsc::mdc::model::ESecurityType security_type) {
    int8_t type = 0;
    switch (security_type) {
        case ESecurityType::StockType:
        case ESecurityType::FundType:
        case ESecurityType::BondType:
            type = kDTypeStock;
            break;
        case ESecurityType::IndexType:
            type = kDTypeIndex;
            break;
        case ESecurityType::OptionType:
            type = kDTypeOption;
            break;
        case ESecurityType::FuturesType:
            type = kDTypeFuture;
            break;
        default:
            break;
    }
    return type;
}
}  // namespace co
