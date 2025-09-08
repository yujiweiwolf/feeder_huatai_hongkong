// Copyright 2025 Fancapital Inc.  All rights reserved.
#include "yaml-cpp/yaml.h"
#include "config.h"

namespace co {

std::shared_ptr<Config> Config::Load() {
    auto getStr = [&](const YAML::Node& node, const std::string& name) {
        try {
            return node[name] && !node[name].IsNull() ? node[name].as<std::string>() : "";
        } catch (std::exception& e) {
            LOG_ERROR << "load configuration failed: name = " << name << ", error = " << e.what();
            throw std::runtime_error(e.what());
        }
    };
    auto getStrings = [&](std::vector<std::string>* ret, const YAML::Node& node, const std::string& name, bool drop_empty = false) {
        try {
            if (node[name] && !node[name].IsNull()) {
                for (auto item : node[name]) {
                    std::string s = x::Trim(item.as<std::string>());
                    if (!drop_empty || !s.empty()) {
                        ret->emplace_back(s);
                    }
                }
            }
        } catch (std::exception& e) {
            LOG_ERROR << "load configuration failed: name = " << name << ", error = " << e.what();
            throw std::runtime_error(e.what());
        }
    };
    auto getInt = [&](const YAML::Node& node, const std::string& name) {
        try {
            return node[name] && !node[name].IsNull() ? node[name].as<int64_t>() : 0;
        } catch (std::exception& e) {
            LOG_ERROR << "load configuration failed: name = " << name << ", error = " << e.what();
            throw std::runtime_error(e.what());
        }
    };
    auto getBool = [&](const YAML::Node& node, const std::string& name) {
        try {
            return node[name] && !node[name].IsNull() ? node[name].as<bool>() : false;
        } catch (std::exception& e) {
            LOG_ERROR << "load configuration failed: name = " << name << ", error = " << e.what();
            throw std::runtime_error(e.what());
        }
    };
    auto opt = std::make_shared<Config>();
    std::string filename = x::FindFile("feeder.yaml");
    auto log_opt = x::LoggingOptions::Load(filename);
    x::InitializeLogging(*log_opt);
    YAML::Node root = YAML::LoadFile(filename);
    auto insight_feeder = root["insight"];
    opt->opt_ = FeedOptions::Load(filename);
    opt->server_host_ = getStr(insight_feeder, "server_host");
    opt->server_port_ = getInt(insight_feeder, "server_port");
    opt->server_host_1_ = getStr(insight_feeder, "server_host_1");
    opt->server_port_1_ = getInt(insight_feeder,  "server_port_1");
    opt->username_ = getStr(insight_feeder, "username");
    std::string password = getStr(insight_feeder, "password");
    opt->password_ = DecodePassword(password);
    opt->api_log_flag_ = getInt(insight_feeder,  "apilog_flag");
    opt->interface_ip_ = getStr(insight_feeder, "interface_ip");
    opt->cpu_affinity_ = getInt(insight_feeder,  "cpu_affinity");

    auto tcp_insight = root["tcp_insight"];
    opt->tcp_server_host_ = getStr(tcp_insight, "server_host");
    opt->tcp_server_port_ = getInt(tcp_insight,  "server_port");
    opt->tcp_username_ = getStr(tcp_insight, "username");
    password = getStr(tcp_insight, "password");
    opt->tcp_password_ = DecodePassword(password);
    return opt;
}

std::string Config::ToString() const {
    std::stringstream ss;
    ss << opt_->ToString() << std::endl
       << "tcp insight:" << std::endl
       << "  tcp_server_host: " << tcp_server_host_ << std::endl
       << "  tcp_server_port: " << tcp_server_port_ << std::endl
       << "  tcp_username: " << tcp_username_ << std::endl
       << "  tcp_password: " << std::string(tcp_password_.size(), '*') << endl
       << "insight:" << std::endl
       << "  server_host: " << server_host_ << std::endl
       << "  server_port: " << server_port_ << std::endl
       << "  server_host_1: " << server_host_1_ << std::endl
       << "  server_port_1: " << server_port_1_ << std::endl
       << "  username: " << username_ << std::endl
       << "  password: " << std::string(password_.size(), '*') << endl
       << "  interface_ip: " << interface_ip_ << std::endl
       << "  api_log_flag: " << api_log_flag_ << std::endl
       << "  cpu_affinity: " << cpu_affinity_ << std::endl;
    return ss.str();
}
}  // namespace co
