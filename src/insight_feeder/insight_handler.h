// Copyright 2025 Fancapital Inc.  All rights reserved.
#pragma once
#include <boost/lockfree/queue.hpp>
#include "config.h"
#include "insight_support.h"

namespace co {

class InsightHandler : public MessageHandle {
 public:
    explicit InsightHandler(std::shared_ptr<FeedServer> feed_server);
    ~InsightHandler();

    void Init(std::shared_ptr<Config> opt);
    void Start();
    void Run();

    /**
    * 发送订阅请求后服务端回复消息，查看是否订阅成功
    * @param[in] data_stream
    */
    void OnServiceMessage(const ::com::htsc::mdc::insight::model::MarketDataStream& data_stream);

    /**
    * 处理订阅后推送的实时行情数据
    * @param[in] data
    */
    void OnMarketData(const com::htsc::mdc::insight::model::MarketData& data);

    /**
    * 处理回测请求成功后推送的回测数据
    * @param[in] PlaybackPayload 回测数据
    */
    void OnPlaybackPayload(const PlaybackPayload& payload);

    /**
    * 处理回测状态
    * @param[in] PlaybackStatus 回测状态
    */
    void OnPlaybackStatus(const com::htsc::mdc::insight::model::PlaybackStatus& status);

    /**
    * 处理查询请求返回结果
    * @param[in] MDQueryResponse 查询请求返回结果
    */
    void OnQueryResponse(const ::com::htsc::mdc::insight::model::MDQueryResponse& response);

    void OnLoginSuccess();

    void OnLoginFailed(int error_no, const std::string& message);

    void OnNoConnections();

    void OnReconnect();

 protected:
    void Parse(const MDStock& p);
    void Parse(const MDFund& p);
    void Parse(const MDBond& p);
    void Parse(const MDIndex& p);
    void Parse(const MDFuture& p);
    void Parse(const MDOption& p);
    void Parse(const MDOrder& p);
    void Parse(const MDTransaction& p);

 private:
    std::shared_ptr<Config> opt_ = nullptr;
    boost::lockfree::queue<com::htsc::mdc::insight::model::MarketData*, boost::lockfree::fixed_sized<false>> queue_;
    std::unique_ptr<std::thread> thread_ = nullptr;
    std::shared_ptr<FeedServer> feed_server_ = nullptr;
    set<double> all_price_step_;
};
}  // namespace co
