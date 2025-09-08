// Copyright 2025 Fancapital Inc.  All rights reserved.
#include <numeric>
#include "insight_handler.h"
namespace co {
InsightHandler::InsightHandler(std::shared_ptr<FeedServer> feed_server): queue_(100000), feed_server_(feed_server) {
    all_price_step_ = {0.001, 0.002, 0.005, 0.01, 0.02, 0.05, 0.10, 0.20, 0.50, 1};
}

InsightHandler::~InsightHandler() {
}

void InsightHandler::OnServiceMessage(const ::com::htsc::mdc::insight::model::MarketDataStream& ds) {
    // pass
}

void InsightHandler::Init(std::shared_ptr<Config> opt) {
    opt_ = opt;
}

void InsightHandler::Start() {
    thread_ = std::make_unique<std::thread>(std::bind(&InsightHandler::Run, this));
}

void InsightHandler::Run() {
    if (opt_->cpu_affinity() > 0) {
        LOG_INFO << "insight handler bind cpu: " << opt_->cpu_affinity();
        x::SetCPUAffinity(opt_->cpu_affinity());
    }
    while (true) {
        com::htsc::mdc::insight::model::MarketData* out;
        while (true) {
            if (queue_.pop(out)) {
                break;
            }
        }
        if (!out) {
            continue;
        }
        const com::htsc::mdc::insight::model::MarketData& data = *out;
        switch (data.marketdatatype()) {
            case MD_CONSTANT: {
                break;
            }
            case MD_TICK: {  // 快照
                if (data.has_mdstock()) {  // 股票
                    const MDStock& p = data.mdstock();
                    Parse(p);
                } else if (data.has_mdbond()) {  // 债券
                    const MDBond& p = data.mdbond();
                    Parse(p);
                } else if (data.has_mdindex()) {  // 指数
                    const MDIndex& p = data.mdindex();
                    Parse(p);
                } else if (data.has_mdfund()) {  // 基金
                    const MDFund& p = data.mdfund();
                    Parse(p);
                } else if (data.has_mdoption()) {  // 期权
                    const MDOption& p = data.mdoption();
                    Parse(p);
                } else if (data.has_mdfuture()) {  // 期货
                    const MDFuture& p = data.mdfuture();
                    Parse(p);
                } else {
                }
                break;
            }
            case MD_ORDER: {  // 逐笔委托
                if (data.has_mdorder()) {
                    const MDOrder& p = data.mdorder();
                    Parse(p);
                }
                break;
            }
            case MD_TRANSACTION: {  // 逐笔成交
                if (data.has_mdtransaction()) {
                    const MDTransaction& p = data.mdtransaction();
                    Parse(p);
                }
                break;
            }
            default:
                break;
        }
        delete out;
    }
}

void InsightHandler::OnMarketData(const com::htsc::mdc::insight::model::MarketData& data) {
    auto item = new com::htsc::mdc::insight::model::MarketData();
    item->CopyFrom(data);
    queue_.push(item);
}

void InsightHandler::OnPlaybackPayload(const PlaybackPayload& payload) {
}

void InsightHandler::OnPlaybackStatus(const com::htsc::mdc::insight::model::PlaybackStatus& status) {
    LOG_INFO << "OnPlaybackStatus: " << status.Utf8DebugString();
}

void InsightHandler::OnQueryResponse(const ::com::htsc::mdc::insight::model::MDQueryResponse& response) {
}

void InsightHandler::OnLoginSuccess() {
    LOG_INFO << "login success";
}

void InsightHandler::OnLoginFailed(int error_no, const std::string& message) {
    LOG_ERROR << "login failed: " << error_no << "-" << message;
}

void InsightHandler::OnNoConnections() {
    // 处理所有服务器都无法连接的情况
    LOG_INFO << "OnNoConnections";
}

void InsightHandler::OnReconnect() {
    LOG_INFO << "reconnect ...";
}

int Calculate_Gcd(const std::vector<int>& numbers) {
    if (numbers.empty()) {
        return 10000;  // 没有盘口，返回最大值 1 * 10000
    }
    int result = numbers[0];
    for (size_t i = 1; i < numbers.size(); ++i) {
        result = std::gcd(result, numbers[i]);
        if (result == 1) break;
    }
    if (result > 10000) {
        result = 10000;
    }
    return result;
}

int Calculate_Tick(MemQTickBody* body) {
    std::vector<int> numbers;
    for (int i = 0; i < 10; i++) {
        if (body->ap[i] > 0) {
            numbers.push_back(f2i(body->ap[i]));
        } else {
            break;
        }
    }
    for (int i = 0; i < 10; i++) {
        if (body->bp[i] > 0) {
            numbers.push_back(f2i(body->bp[i]));
        } else {
            break;
        }
    }
    return Calculate_Gcd(numbers);
}

void InsightHandler::Parse(const MDStock& p) {
    const std::string& std_code = p.htscsecurityid();
    int64_t market = Market2Std(p.securityidsource());
    int64_t date = p.exchangedate() > 0 ? p.exchangedate() : p.mddate();
    int64_t stamp = p.exchangetime() > 0 ? p.exchangetime() : p.mdtime();
    int64_t timestamp = date * 1000000000LL + stamp % 1000000000LL;
    double pre_close = i2f(p.preclosepx());
    double close = i2f(p.closepx());
    double upper_limit = i2f(p.maxpx());
    double lower_limit = i2f(p.minpx());
    MemQTickBody body {};
    {
        strncpy(body.code, std_code.data(), std_code.size());
        body.timestamp = timestamp;
        body.sum_volume = p.totalvolumetrade();
        body.sum_amount = (double)p.totalvaluetrade();
        body.new_price = i2f(p.lastpx());
        body.high = i2f(p.highpx());
        body.low = i2f(p.lowpx());
        body.open = i2f(p.openpx());

        for (int i = 0; i < 10 && i < p.buyorderqtyqueue_size() && i < p.buypricequeue_size(); ++i) {
            int64_t price = p.buypricequeue(i);
            int64_t volume = p.buyorderqtyqueue(i);
            if ((volume > 0 && price > 0) || (i == 1 && volume > 0 && price == 0)) {
                body.bp[i] = i2f(price);
                body.bv[i] = volume;
            } else {
                break;
            }
        }
        for (int i = 0; i < 10 && i < p.sellorderqtyqueue_size() && i < p.sellpricequeue_size(); ++i) {
            int64_t price = p.sellpricequeue(i);
            int64_t volume = p.sellorderqtyqueue(i);
            if ((volume > 0 && price > 0) || (i == 1 && volume > 0 && price == 0)) {
                body.ap[i] = i2f(price);
                body.av[i] = volume;
            } else {
                break;
            }
        }
        int64_t state = Status2Std(p.tradingphasecode());
        body.state = state;
    }
    double price_step = i2f(Calculate_Tick(&body));
    bool change_flag = false;
    FeedContext* ctx = nullptr;
    auto tick = feed_server_->CreateQTickBody(std_code.c_str(), &ctx);
    if (!tick) {
        return;
    }
    auto ctx_head = ctx->GetQTickHead();
    if (x::Eq(ctx_head->price_step, 0)) {
        change_flag = true;
    } else {
        // 变小后再次计算最大公约数
        if (price_step < ctx_head->price_step) {
            std::vector<int> numbers;
            numbers.push_back(f2i(price_step));
            numbers.push_back(f2i(ctx_head->price_step));
            double new_price_step = i2f(Calculate_Gcd(numbers));
            LOG_INFO << std_code << ", " << timestamp << ", price step change small, pre: " << ctx_head->price_step
                     << ", now: " << price_step << ", new: " << new_price_step;
            auto it = all_price_step_.find(new_price_step);
            if (it != all_price_step_.end()) {
                change_flag = true;
                price_step = new_price_step;
            } else {
                LOG_ERROR << "not valid new price_step: " << new_price_step;
            }
        }
    }
    if (ctx_head->timestamp <= 0 ||
        (pre_close > 0 && x::Ne(pre_close, ctx_head->pre_close)) ||
        (close > 0 && x::Ne(close, ctx_head->close)) || change_flag ||
        (upper_limit > 0 && x::Ne(upper_limit, ctx_head->upper_limit)) ||
        (lower_limit > 0 && x::Ne(lower_limit, ctx_head->lower_limit))) {
        auto head = feed_server_->CreateQTickHead(std_code.c_str(), &ctx);
        memcpy(head, ctx_head, sizeof(*ctx_head));
        head->market = market;
        head->timestamp = timestamp;
        head->dtype = kDTypeStock;
        head->pre_close = pre_close;
        head->close = close;
        head->upper_limit = upper_limit;
        head->lower_limit = lower_limit;
        if (change_flag) {
            head->price_step = price_step;
        }
        LOG_INFO << ToString(head);
        feed_server_->PushQTickHead(ctx, head);
    }
    memcpy(tick, &body, sizeof(body));
    feed_server_->PushQTickBody(ctx, tick);
}

void InsightHandler::Parse(const MDFund& p) {
    const std::string& std_code = p.htscsecurityid();
    int64_t market = Market2Std(p.securityidsource());
    int64_t date = p.exchangedate() > 0 ? p.exchangedate() : p.mddate();
    int64_t stamp = p.exchangetime() > 0 ? p.exchangetime() : p.mdtime();
    int64_t timestamp = date * 1000000000LL + stamp % 1000000000LL;
    double pre_close = i2f(p.preclosepx());
    double close = i2f(p.closepx());
    double upper_limit = i2f(p.maxpx());
    double lower_limit = i2f(p.minpx());
    FeedContext* ctx = nullptr;
    auto tick = feed_server_->CreateQTickBody(std_code.c_str(), &ctx);
    if (!tick) {
        return;
    }
    auto ctx_head = ctx->GetQTickHead();
    if (ctx_head->timestamp <= 0 ||
        (pre_close > 0 && x::Ne(pre_close, ctx_head->pre_close)) ||
        (close > 0 && x::Ne(close, ctx_head->close)) ||
        (upper_limit > 0 && x::Ne(upper_limit, ctx_head->upper_limit)) ||
        (lower_limit > 0 && x::Ne(lower_limit, ctx_head->lower_limit))) {
        auto head = feed_server_->CreateQTickHead(std_code.c_str(), &ctx);
        memcpy(head, ctx_head, sizeof(*ctx_head));
        head->market = market;
        head->timestamp = timestamp;
        head->dtype = kDTypeStock;
        head->pre_close = pre_close;
        head->close = close;
        head->upper_limit = upper_limit;
        head->lower_limit = lower_limit;
        feed_server_->PushQTickHead(ctx, head);
    }

    strncpy(tick->code, std_code.c_str(), std_code.size());
    tick->timestamp = timestamp;
    tick->sum_volume = p.totalvolumetrade();
    tick->sum_amount = (double)p.totalvaluetrade();
    tick->new_price = i2f(p.lastpx());
    tick->high = i2f(p.highpx());
    tick->low = i2f(p.lowpx());
    tick->open = i2f(p.openpx());

    for (int i = 0; i < 10 && i < p.buyorderqtyqueue_size() && i < p.buypricequeue_size(); ++i) {
        int64_t price = p.buypricequeue(i);
        int64_t volume = p.buyorderqtyqueue(i);
        if ((volume > 0 && price > 0) || (i == 1 && volume > 0 && price == 0)) {
            tick->bp[i] = i2f(price);;
            tick->bv[i] = volume;
        } else {
            break;
        }
    }
    for (int i = 0; i < 10 && i < p.sellorderqtyqueue_size() && i < p.sellpricequeue_size(); ++i) {
        int64_t price = p.sellpricequeue(i);
        int64_t volume = p.sellorderqtyqueue(i);
        if ((volume > 0 && price > 0) || (i == 1 && volume > 0 && price == 0)) {
            tick->ap[i] = i2f(price);
            tick->av[i] = volume;
        } else {
            break;
        }
    }
    tick->state = Status2Std(p.tradingphasecode());
    feed_server_->PushQTickBody(ctx, tick);

    if (p.purchasenumber() > 0 || p.redemptionnumber() > 0) {
        MemQEtfTick* tick = feed_server_->CreateQEtfTick(std_code.c_str(), &ctx);
        auto etf_data = (MemQEtfTick*)ctx->user_data();
        if (etf_data == nullptr) {
            etf_data = new MemQEtfTick();
            etf_data->sum_create_count = 0;
            etf_data->sum_create_volume = 0;
            etf_data->sum_redeem_count = 0;
            etf_data->sum_redeem_volume = 0;
            ctx->set_user_data(etf_data);
        }
        strncpy(tick->code, std_code.c_str(), std_code.size());
        tick->timestamp = timestamp;
        tick->sum_create_count = p.purchasenumber();
        tick->sum_create_volume = p.purchaseamount();
        tick->sum_redeem_count = p.redemptionnumber();
        tick->sum_redeem_volume = p.redemptionamount();
        tick->new_create_count = tick->sum_create_count - etf_data->sum_create_count;
        tick->new_create_volume = tick->sum_create_volume - etf_data->sum_create_volume;
        tick->new_redeem_count = tick->sum_redeem_count - etf_data->sum_redeem_count;
        tick->new_redeem_volume = tick->sum_redeem_volume - etf_data->sum_redeem_volume;
        etf_data->timestamp = timestamp;
        etf_data->sum_create_count = tick->sum_create_count;
        etf_data->sum_create_volume = tick->sum_create_volume;
        etf_data->sum_redeem_count = tick->sum_redeem_count;
        etf_data->sum_redeem_volume = tick->sum_redeem_volume;
        feed_server_->PushQEtfTick(ctx, tick);
    }
}

void InsightHandler::Parse(const MDBond& p) {
    const std::string& std_code = p.htscsecurityid();
    int64_t market = Market2Std(p.securityidsource());
    int64_t date = p.exchangedate() > 0 ? p.exchangedate() : p.mddate();
    int64_t stamp = p.exchangetime() > 0 ? p.exchangetime() : p.mdtime();
    int64_t timestamp = date * 1000000000LL + stamp % 1000000000LL;
    double pre_close = i2f(p.preclosepx());
    double close = i2f(p.closepx());
    double upper_limit = i2f(p.maxpx());
    double lower_limit = i2f(p.minpx());

    FeedContext* ctx = nullptr;
    auto tick = feed_server_->CreateQTickBody(std_code.c_str(), &ctx);
    if (!tick) {
        return;
    }
    auto ctx_head = ctx->GetQTickHead();
    if (ctx_head->timestamp <= 0 ||
        (pre_close > 0 && x::Ne(pre_close, ctx_head->pre_close)) ||
        (close > 0 && x::Ne(close, ctx_head->close)) ||
        (upper_limit > 0 && x::Ne(upper_limit, ctx_head->upper_limit)) ||
        (lower_limit > 0 && x::Ne(lower_limit, ctx_head->lower_limit))) {
        auto head = feed_server_->CreateQTickHead(std_code.c_str(), &ctx);
        memcpy(head, ctx_head, sizeof(*ctx_head));
        head->market = market;
        head->timestamp = timestamp;
        head->dtype = kDTypeStock;
        head->pre_close = pre_close;
        head->close = close;
        head->upper_limit = upper_limit;
        head->lower_limit = lower_limit;
        feed_server_->PushQTickHead(ctx, head);
    }

    int64_t volume_unit = ctx_head->volume_unit > 0 ? ctx_head->volume_unit : 1;
    strncpy(tick->code, std_code.c_str(), std_code.length());
    tick->timestamp = timestamp;
    tick->sum_volume = p.totalvolumetrade() * volume_unit;
    tick->sum_amount = (double) p.totalvaluetrade();
    tick->new_price = i2f(p.lastpx());
    tick->high = i2f(p.highpx());
    tick->low = i2f(p.lowpx());
    tick->open = i2f(p.openpx());

    for (int i = 0; i < 10 && i < p.buyorderqtyqueue_size() && i < p.buypricequeue_size(); ++i) {
        int64_t price = p.buypricequeue(i);
        int64_t volume = p.buyorderqtyqueue(i);
        if ((volume > 0 && price > 0) || (i == 1 && volume > 0 && price == 0)) {
            tick->bp[i] = i2f(price);;
            tick->bv[i] = volume * volume_unit;
        } else {
            break;
        }
    }
    for (int i = 0; i < 10 && i < p.sellorderqtyqueue_size() && i < p.sellpricequeue_size(); ++i) {
        int64_t price = p.sellpricequeue(i);
        int64_t volume = p.sellorderqtyqueue(i);
        if ((volume > 0 && price > 0) || (i == 1 && volume > 0 && price == 0)) {
            tick->ap[i] = i2f(price);
            tick->av[i] = volume * volume_unit;
        } else {
            break;
        }
    }
    tick->state = Status2Std(p.tradingphasecode());
    feed_server_->PushQTickBody(ctx, tick);
}

// 无CSI的静态信息
void InsightHandler::Parse(const MDIndex& p) {
    const std::string& std_code = p.htscsecurityid();
    int64_t market = Market2Std(p.securityidsource());
    int64_t date = p.exchangedate() > 0 ? p.exchangedate() : p.mddate();
    int64_t stamp = p.exchangetime() > 0 ? p.exchangetime() : p.mdtime();
    int64_t timestamp = date * 1000000000LL + stamp % 1000000000LL;

    double pre_close = i2f(p.preclosepx());
    double close = i2f(p.closepx());
    double upper_limit = i2f(p.maxpx());
    double lower_limit = i2f(p.minpx());

    FeedContext* ctx = nullptr;
    auto tick = feed_server_->CreateQTickBody(std_code.c_str(), &ctx);
    if (!tick) {
        return;
    }
    auto ctx_head = ctx->GetQTickHead();
    if (ctx_head->timestamp <= 0 ||
        (pre_close > 0 && x::Ne(pre_close, ctx_head->pre_close)) ||
        (close > 0 && x::Ne(close, ctx_head->close)) ||
        (upper_limit > 0 && x::Ne(upper_limit, ctx_head->upper_limit)) ||
        (lower_limit > 0 && x::Ne(lower_limit, ctx_head->lower_limit))) {
        auto head = feed_server_->CreateQTickHead(std_code.c_str(), &ctx);
        memcpy(head, ctx_head, sizeof(*ctx_head));
        head->market = market;
        head->timestamp = timestamp;
        head->dtype = kDTypeIndex;
        head->pre_close = pre_close;
        head->close = close;
        head->upper_limit = upper_limit;
        head->lower_limit = lower_limit;
        feed_server_->PushQTickHead(ctx, head);
    }

    auto pre_tick = (MemQTickBody*)ctx->user_data();
    if (pre_tick == nullptr) {
        pre_tick = new MemQTickBody();
        pre_tick->timestamp = 0;
        pre_tick->new_price = 0;
        pre_tick->sum_volume = 0;
        ctx->set_user_data(pre_tick);
    }

    if (timestamp <= pre_tick->timestamp) {
        return;
    }

    if (pre_tick->timestamp > 0 && p.lastpx() == pre_tick->new_price && p.totalvaluetrade() <= pre_tick->sum_volume) {
        return;
    }

    pre_tick->timestamp = timestamp;
    pre_tick->new_price = p.lastpx();
    pre_tick->sum_volume = p.totalvolumetrade();

    strncpy(tick->code, std_code.c_str(), std_code.size());
    tick->timestamp = timestamp;
    tick->sum_volume = p.totalvolumetrade() / 100;
    tick->sum_amount = (double)p.totalvaluetrade();
    tick->new_price = i2f(p.lastpx());
    tick->high = i2f(p.highpx());
    tick->low = i2f(p.lowpx());
    tick->open = i2f(p.openpx());
    tick->state = Status2Std(p.tradingphasecode());
    feed_server_->PushQTickBody(ctx, tick);
}

// 只有中金所期货数据
void InsightHandler::Parse(const MDFuture& p) {
    int64_t market = Market2Std(p.securityidsource());
    if (market == 0) {
        return;
    }
    string_view securityid = p.htscsecurityid();
    auto pos = securityid.find('.');
    if (pos == securityid.npos) {
        return;
    }
    string_view code(securityid.data(), pos);
    string std_code = string(code) + string(MarketToSuffix(market));
    int64_t date = p.exchangedate() > 0 ? p.exchangedate() : p.mddate();
    int64_t stamp = p.exchangetime() > 0 ? p.exchangetime() : p.mdtime();
    int64_t timestamp = date * 1000000000LL + stamp % 1000000000LL;

    double pre_close = i2f(p.preclosepx());
    double close = i2f(p.closepx());
    double pre_settle = i2f(p.presettleprice());
    double settle = i2f(p.settleprice());
    double upper_limit = i2f(p.maxpx());
    double lower_limit = i2f(p.minpx());

    FeedContext* ctx = nullptr;
    auto tick = feed_server_->CreateQTickBody(std_code.c_str(), &ctx);
    if (!tick) {
        return;
    }
    auto ctx_head = ctx->GetQTickHead();
    if (ctx_head->timestamp <= 0 ||
        (pre_close > 0 && x::Ne(pre_close, ctx_head->pre_close)) ||
        (close > 0 && x::Ne(close, ctx_head->close)) ||
        (pre_settle > 0 && x::Ne(pre_settle, ctx_head->pre_settle)) ||
        (settle > 0 && x::Ne(settle, ctx_head->settle)) ||
        (upper_limit > 0 && x::Ne(upper_limit, ctx_head->upper_limit)) ||
        (lower_limit > 0 && x::Ne(lower_limit, ctx_head->lower_limit))) {
        auto head = feed_server_->CreateQTickHead(std_code.c_str(), &ctx);
        memcpy(head, ctx_head, sizeof(*ctx_head));
        head->market = market;
        head->timestamp = timestamp;
        head->dtype = kDTypeFuture;
        head->pre_close = pre_close;
        head->close = close;
        head->pre_settle = pre_settle;
        head->settle = settle;
        head->upper_limit = upper_limit;
        head->lower_limit = lower_limit;
        head->pre_open_interest = p.preopeninterest();
        feed_server_->PushQTickHead(ctx, head);
    }

    strncpy(tick->code, std_code.c_str(), std_code.length());
    tick->timestamp = timestamp;
    tick->sum_volume = p.totalvolumetrade();
    tick->sum_amount = (double)p.totalvaluetrade();
    tick->new_price = i2f(p.lastpx());
    tick->open_interest = p.openinterest();
    tick->high = i2f(p.highpx());
    tick->low = i2f(p.lowpx());
    tick->open = i2f(p.openpx());
    for (int i = 0; i < 10 && i < p.buyorderqtyqueue_size() && i < p.buypricequeue_size(); ++i) {
        int64_t price = p.buypricequeue(i);
        int64_t volume = p.buyorderqtyqueue(i);
        if ((volume > 0 && price > 0) || (i == 1 && volume > 0 && price == 0)) {
            tick->bp[i] = i2f(price);;
            tick->bv[i] = volume;
        } else {
            break;
        }
    }
    for (int i = 0; i < 10 && i < p.sellorderqtyqueue_size() && i < p.sellpricequeue_size(); ++i) {
        int64_t price = p.sellpricequeue(i);
        int64_t volume = p.sellorderqtyqueue(i);
        if ((volume > 0 && price > 0) || (i == 1 && volume > 0 && price == 0)) {
            tick->ap[i] = i2f(price);
            tick->av[i] = volume;
        } else {
            break;
        }
    }
    tick->state = Status2Std(p.tradingphasecode());
    feed_server_->PushQTickBody(ctx, tick);
}

// 商品期权，中金所期权
void InsightHandler::Parse(const MDOption& p) {
    string std_code;
    int64_t market = Market2Std(p.securityidsource());
    if (market == 0) {
        return;
    }
    if (market == kMarketSZ || market == kMarketSH) {
        std_code = p.htscsecurityid();
    } else {
        string_view securityid = p.htscsecurityid();
        auto pos = securityid.find('.');
        if (pos == securityid.npos) {
            return;
        }
        string_view code(securityid.data(), pos);
        std_code = string(code) + string(MarketToSuffix(market));
    }

    int64_t date = p.exchangedate() > 0 ? p.exchangedate() : p.mddate();
    int64_t stamp = p.exchangetime() > 0 ? p.exchangetime() : p.mdtime();
    int64_t timestamp = date * 1000000000LL + stamp % 1000000000LL;
    double pre_close = i2f(p.preclosepx());
    double close = i2f(p.closepx());
    double pre_settle = i2f(p.presettleprice());
    double settle = i2f(p.settleprice());
    double upper_limit = i2f(p.maxpx());
    double lower_limit = i2f(p.minpx());

    FeedContext* ctx = nullptr;
    auto tick = feed_server_->CreateQTickBody(std_code.c_str(), &ctx);
    if (!tick) {
        return;
    }
    auto ctx_head = ctx->GetQTickHead();
    if (ctx_head->timestamp <= 0 ||
        (pre_close > 0 && x::Ne(pre_close, ctx_head->pre_close)) ||
        (close > 0 && x::Ne(close, ctx_head->close)) ||
        (pre_settle > 0 && x::Ne(pre_settle, ctx_head->pre_settle)) ||
        (settle > 0 && x::Ne(settle, ctx_head->settle)) ||
        (upper_limit > 0 && x::Ne(upper_limit, ctx_head->upper_limit)) ||
        (lower_limit > 0 && x::Ne(lower_limit, ctx_head->lower_limit))) {
        auto head = feed_server_->CreateQTickHead(std_code.c_str(), &ctx);
        memcpy(head, ctx_head, sizeof(*ctx_head));
        head->market = market;
        head->timestamp = timestamp;
        head->dtype = kDTypeOption;
        head->pre_close = pre_close;
        head->close = close;
        head->pre_settle = pre_settle;
        head->settle = settle;
        head->upper_limit = upper_limit;
        head->lower_limit = lower_limit;
        head->pre_open_interest = p.preopeninterest();
        feed_server_->PushQTickHead(ctx, head);
    }

    strncpy(tick->code, std_code.data(), std_code.size());
    tick->timestamp = timestamp;
    tick->sum_volume = p.totalvolumetrade();
    tick->sum_amount = (double)p.totalvaluetrade();
    tick->new_price = i2f(p.lastpx());
    tick->high = i2f(p.highpx());
    tick->low = i2f(p.lowpx());
    tick->open = i2f(p.openpx());
    tick->open_interest = p.openinterest();
    for (int i = 0; i < 10 && i < p.buyorderqtyqueue_size() && i < p.buypricequeue_size(); ++i) {
        int64_t price = p.buypricequeue(i);
        int64_t volume = p.buyorderqtyqueue(i);
        if ((volume > 0 && price > 0) || (i == 1 && volume > 0 && price == 0)) {
            tick->bp[i] = i2f(price);;
            tick->bv[i] = volume;
        } else {
            break;
        }
    }
    for (int i = 0; i < 10 && i < p.sellorderqtyqueue_size() && i < p.sellpricequeue_size(); ++i) {
        int64_t price = p.sellpricequeue(i);
        int64_t volume = p.sellorderqtyqueue(i);
        if ((volume > 0 && price > 0) || (i == 1 && volume > 0 && price == 0)) {
            tick->ap[i] = i2f(price);
            tick->av[i] = volume;
        } else {
            break;
        }
    }
    tick->state = Status2Std(p.tradingphasecode());
    feed_server_->PushQTickBody(ctx, tick);
}

void InsightHandler::Parse(const MDOrder& p) {
    // 11 12 13 14 15等是新债券
    if (p.ordertype() >= 11 && p.ordertype() <= 15) {
        return;
    }
    const auto& code = p.htscsecurityid();
    if (code.size() != 9) {
        return;
    }
    int64_t order_no = 0;
    if (p.securityidsource() == XSHG) {
        order_no = p.orderno();
    } else {
        order_no = p.orderindex();
    }

    int64_t bs_flag = BsFlag2Std(p.orderbsflag());
    int64_t order_type = OrderType2Std(p.ordertype());
    int64_t date = p.exchangedate() > 0 ? p.exchangedate() : p.mddate();
    int64_t stamp = p.exchangetime() > 0 ? p.exchangetime() : p.mdtime();
    int64_t timestamp = date * 1000000000LL + stamp % 1000000000LL;

    FeedContext* ctx = nullptr;
    MemQOrder* order = feed_server_->CreateQOrder(code.c_str(), &ctx);
    if (!order) {
        return;
    }
    auto ctx_head = ctx->GetQTickHead();
    strncpy(order->code, code.c_str(), code.size());
    order->order_volume = ctx_head->volume_unit > 0 ? p.orderqty() * ctx_head->volume_unit : p.orderqty();
    order->order_price = p.orderprice();
    order->order_no = order_no;
    order->timestamp = timestamp;
    order->order_type = order_type;
    order->bs_flag = bs_flag;
    order->stream_no = p.applseqnum();
    feed_server_->PushQOrder(ctx, order);
}

void InsightHandler::Parse(const MDTransaction& p) {
    const auto& code = p.htscsecurityid();
    if (code.length() != 9) {
        return;
    }
    int64_t date = p.exchangedate() > 0 ? p.exchangedate() : p.mddate();
    int64_t stamp = p.exchangetime() > 0 ? p.exchangetime() : p.mdtime();
    int64_t timestamp = date * 1000000000LL + stamp % 1000000000LL;

    FeedContext* ctx = nullptr;
    MemQKnock* knock = feed_server_->CreateQKnock(code.c_str(), &ctx);
    if (!knock) {
        return;
    }
    auto ctx_head = ctx->GetQTickHead();
    strncpy(knock->code, code.c_str(), code.length());
    knock->match_volume = ctx_head->volume_unit > 0 ? p.tradeqty() * ctx_head->volume_unit : p.tradeqty();
    knock->match_price = p.tradeprice();
    knock->ask_order_no = p.tradesellno();
    knock->bid_order_no = p.tradebuyno();
    knock->match_no = p.tradeindex();
    knock->timestamp = timestamp;
    knock->stream_no = p.applseqnum();
    feed_server_->PushQKnock(ctx, knock);
}
}  // namespace co
