// Copyright 2025 Fancapital Inc.  All rights reserved.
#pragma once
#include "config.h"
#include "insight_support.h"
#include "insight_handler.h"

namespace co {
class InsightServer {
 public:
    void Init(std::shared_ptr<Config> opt);
    void Run();
    void Stop();

 protected:
    void Login();
    void SubQuotation();
    void QueryContracts();
    // source 交易所; data_type 数据类型，逐笔还是Tick; sec_type 合约类型，股票还是指数
    void SetSubInfo(SubscribeBySourceType& sub, const ESecurityIDSource& source, const EMarketDataType& data_type, const ESecurityType& sec_type);

 private:
    std::shared_ptr<Config> opt_ = nullptr;
    UdpClientInterface* udp_client_ = nullptr;
    InsightHandler* handler_ = nullptr;
    shared_ptr<FeedServer> feed_server_;
};
}  // namespace co
