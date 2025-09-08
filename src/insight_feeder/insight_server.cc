// Copyright 2025 Fancapital Inc.  All rights reserved.
#include "insight_server.h"
#include "parameter_define.h"

namespace co {
void InsightServer::Init(std::shared_ptr<Config> opt) {
    opt_ = opt;
    LOG_INFO << "configuration loaded:" << std::endl << opt_->ToString();
}

void InsightServer::Run() {
    auto opt = opt_;
    feed_server_ = std::make_shared<FeedServer>();
    feed_server_->Init(opt->opt(), opt->opt()->sub_date());
    feed_server_->Start();
    x::Sleep(3000);
    QueryContracts();  // 查询静态信息
    int api_log_flag = opt->api_log_flag();

    if (api_log_flag) {
        open_file_log();  // api日志输出到文件
    } else {
        close_file_log();
    }

    close_cout_log();
    close_trace();
    close_response_callback();
    close_heartbeat_trace();
    close_compress();
    set_int_property_value(HTSC_INSIGHT_CONFIG_THREAD_SLEEP_TIME, 100);
    init_env();

    udp_client_ = ClientFactory::Instance()->CreateUdpClient();
    if (!udp_client_) {
        LOG_FATAL << "create insight udp client failed";
        ClientFactory::Uninstance();
        return;
    }
    handler_ = new InsightHandler(feed_server_);
    handler_->Init(opt_);
    handler_->Start();
    udp_client_->SetWorkPoolThreadCount(5);
    udp_client_->RegistHandle(handler_);
    Login();
    SubQuotation();
    LOG_INFO << "server startup successfully";
    feed_server_->Wait();
    Stop();
    feed_server_->Stop();
}

void InsightServer::Stop() {
    ClientFactory::Uninstance();
    udp_client_ = nullptr;
    delete handler_;
    handler_ = nullptr;
    fini_env();
}

void InsightServer::Login() {
    string server_host = opt_->server_host();
    int server_port = opt_->server_port();
    string username = opt_->username();
    string password = opt_->password();
    LOG_INFO << "login ...";
    // 添加备用发现网关地址
    std::map<std::string, int> backup_list;
    string server_host_1 = opt_->server_host_1();
    int server_port_1 = opt_->server_port_1();
    if (!server_host_1.empty() && server_port_1 > 0) {
        LOG_INFO << "add back udp address: " << server_host_1;
        backup_list.insert(std::pair<std::string, int>(server_host_1, server_port_1));
    }
    while (true) {
        int rc = udp_client_->LoginById(server_host, server_port, username, password, backup_list);
        if (rc == 0) {
            break;
        }
        error_print("LoginById failed");
        LOG_ERROR << "login failed: " << get_error_code_value(rc) << ", retry in 3s ...";
        x::Sleep(3000);
    }
}

// XSHG 上交所, XSHE 深交所, CNI 国证指数有限公司， CSI 中证指数有限公司
// CCFX 中国金融期货交易所, XSGE 上海期货交易所
// XDCE 大连商品交易所, XZCE 郑州商品交易所
// XBSE 北京证券交易所, HKSC 港股通, XHKG 港交所
void InsightServer::QueryContracts() {
    close_cout_log();
    close_trace();
    open_file_log();
    close_heartbeat_trace();
    close_response_callback();
    init_env();
    string server_host = opt_->tcp_server_host();
    int server_port = opt_->tcp_server_port();
    string username = opt_->tcp_username();
    string password = opt_->tcp_password();
    string cert_dir = "../conf";
    ClientInterface* tcp_client = ClientFactory::Instance()->CreateClient(true, cert_dir.c_str());
    if (!tcp_client) {
        LOG_ERROR << "create insight client failed";
        ClientFactory::Uninstance();
        return;
    }
    MessageHandle* handler = new MessageHandle();
    tcp_client->RegistHandle(handler);
    tcp_client->set_handle_pool_thread_count(1);
    LOG_INFO << "tcp login";
    while (true) {
        int rc = tcp_client->LoginByServiceDiscovery(server_host, server_port, username, password, false);
        if (rc == 0) {
            break;
        }
        LOG_ERROR << "login failed: " << get_error_code_value(rc) << ", retry in 10s";
        x::Sleep(10000);
    }
    LOG_INFO << "handle_pool_thread_count: " << tcp_client->handle_pool_thread_count();

    int total_num = 0;
    std::vector<::com::htsc::mdc::model::ESecurityIDSource> all_market;
    all_market.push_back(XHKG);

    for (auto& it : all_market) {
        std::unique_ptr<MDQueryRequest> request(new MDQueryRequest());
        request->set_querytype(QUERY_TYPE_LATEST_BASE_INFORMATION);
        SecuritySourceType* security_source_type = request->add_securitysourcetype();
        security_source_type->set_securityidsource(it);
        std::vector<MDQueryResponse*>* responses = nullptr;
        LOG_INFO << "query contracts, market: " << it;
        // XSHG XSHE 一直查询，直到成功; 其它只查5次
        int index = 0;
        while (true) {
            int rc = tcp_client->RequestMDQuery(&(*request), responses);
            index++;
            if (rc != 0) {
                LOG_ERROR << "query contracts failed: " << get_error_code_value(rc) << ", market: " << it << " , retry in 10s";
                x::Sleep(10000);
                if (it != XSHG && it != XSHE && index >= 5 ) {
                    break;
                }
            } else {
                break;
            }
        }
        if (responses == nullptr) {
            continue;
        }

        for (unsigned int i = 0; i < responses->size(); ++i) {
            if (!responses->at(i)->issuccess()) {
                continue;
            }
            if (!responses->at(i)->has_marketdatastream()) {
                continue;
            }
            auto& datalist = responses->at(i)->marketdatastream().marketdatalist().marketdatas();
            for (auto it =datalist.begin(); it != datalist.end(); ++it) {
                if (it->marketdatatype() == MD_CONSTANT && it->has_mdconstant()) {
                    const MDBasicInfo& p = it->mdconstant();
                    // LOG_INFO << p.Utf8DebugString();  // 打印api全部字段
                    if (p.securitytype() == ESecurityType::OptionType || p.securitytype() == ESecurityType::FuturesType) {
                        if (p.ticksize() <= 0 && p.optionticksize() <= 0) {
                            continue;
                        }
                    }
                    int64_t market = Market2Std(p.securityidsource());
                    string_view security_id = p.htscsecurityid();
                    auto pos = security_id.find('.');
                    if (pos == security_id.npos) {
                        continue;
                    }
                    string_view code(security_id.data(), pos);
                    string std_code = string(code) + string(MarketToSuffix(market));

                    int8_t type = Type2Std(p.securitytype());
                    if (type == 0) {
                        continue;
                    }

                    // 无 pre_close upper_limit lower_limit
                    FeedContext* ctx = nullptr;
                    auto head = feed_server_->CreateQTickHead(std_code.c_str(), &ctx);
                    std::string name = x::Trim(p.symbol());
                    strncpy(head->code, std_code.c_str(), std_code.size());
                    strncpy(head->name, name.c_str(), name.size());
                    head->market = market;
                    head->timestamp = x::RawDate() * 1000000000LL;
                    head->dtype = type;
                    // 204001.SH 被认为是 ESecurityType::BondType
                    if (market == co::kMarketSH && p.securitytype() == ESecurityType::BondType && std_code[0] == '1') {
                        head->volume_unit = 10;
                    } else {
                        head->volume_unit = 0;
                    }
                    if (p.securitytype() == ESecurityType::FuturesType) {
                        head->multiple = p.volumemultiple() >= 0 ? p.volumemultiple() : 0;
                        head->price_step = p.ticksize();
                        head->list_date = atoll(p.listdate().c_str());
                        head->expire_date = atoll(p.expiredate().c_str());
                    } else if (p.securitytype() == ESecurityType::OptionType) {
                        head->multiple = p.optioncontractmultiplierunit() >= 0 ? p.optioncontractmultiplierunit() : 0;
                        head->price_step = p.optionticksize();
                        head->list_date = atoll(p.listdate().c_str());
                        head->expire_date = atoll(p.optionenddate().c_str());
                        head->exercise_price = p.optionexerciseprice();
                        std::string underlying_code = x::Trim(p.optionunderlyingsecurityid());
                        if (!underlying_code.empty()) {
                            underlying_code.append(std::string(MarketToSuffix(market)));
                            strncpy(head->underlying_code, underlying_code.data(), underlying_code.size());
                        }
                        head->cp_flag = CpFlag2Std(x::Trim(p.optioncallorput()));
                    }
                    feed_server_->PushQTickHead(ctx, head);
                    total_num++;
                }
            }
        }
        tcp_client->ReleaseQueryResult(responses);
        LOG_INFO << "query contracts ok: contracts: " << total_num;
    }
    ClientFactory::Uninstance();
    delete handler;
    x::Sleep(2000);
    LOG_INFO << "finish query instrument";
}

void InsightServer::SubQuotation() {
    // 订阅行情
    LOG_INFO << "sub quotation ...";
    std::unique_ptr<SubscribeBySourceType> subscribe_all(new SubscribeBySourceType());
    auto opt = opt_->opt();

    //* 根据证券数据来源订阅行情数据, 由三部分确定行情数据
    //* 行情源(SecurityIdSource) :XSHG(沪市) | XSHE(深市) | ...，不填默认全选
    //* 证券类型(SecurityType) : BondType(债) | StockType(股) | FundType(基) | IndexType(指) | OptionType(期权) | ...
    //* 数据类型(MarketDataTypes) : MD_TICK(快照) | MD_TRANSACTION(逐笔成交) | MD_ORDER(逐笔委托) | ...
    SetSubInfo(*subscribe_all, XHKG, MD_TICK, StockType);
    SetSubInfo(*subscribe_all, XHKG, MD_TICK, FuturesType);

    string interface_ip = opt_->interface_ip();
    LOG_INFO << "interface_ip: " << interface_ip;
    while (true) {
        int rc = udp_client_->SubscribeBySourceType(interface_ip, &(*subscribe_all));
        if (rc == 0) {
            LOG_INFO << "sub quotation ok";
            break;
        }
        LOG_ERROR << "sub quotation failed: " << get_error_code_value(rc) << ", retry in 3s ...";
        x::Sleep(3000);
    }
}

void InsightServer::SetSubInfo(SubscribeBySourceType& sub, const ESecurityIDSource& source, const EMarketDataType& data_type, const ESecurityType& sec_type) {
    SubscribeBySourceTypeDetail* detail = sub.add_subscribebysourcetypedetail();
    SecuritySourceType* sub_info = new SecuritySourceType();
    sub_info->set_securityidsource(source);
    sub_info->set_securitytype(sec_type);
    detail->set_allocated_securitysourcetypes(sub_info);
    detail->add_marketdatatypes(data_type);
}
}  // namespace co
