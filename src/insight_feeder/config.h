// Copyright 2025 Fancapital Inc.  All rights reserved.
#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include "x/x.h"
#include "coral/coral.h"
#include "feeder/feeder.h"

using std::string;

namespace co {
class Config {
 public:
    static std::shared_ptr<Config> Load();
    std::string ToString() const;

    inline string tcp_server_host() {
        return tcp_server_host_;
    }
    inline int tcp_server_port() {
        return tcp_server_port_;
    }
    inline string tcp_username() {
        return tcp_username_;
    }
    inline string tcp_password() {
        return tcp_password_;
    }
    inline string server_host() {
        return server_host_;
    }
    inline int server_port() {
        return server_port_;
    }
    inline string server_host_1() {
        return server_host_1_;
    }
    inline int server_port_1() {
        return server_port_1_;
    }
    inline string username() {
        return username_;
    }
    inline string password() {
        return password_;
    }
    inline int api_log_flag() {
        return api_log_flag_;
    }

    inline string interface_ip() {
        return interface_ip_;
    }
    inline int cpu_affinity() {
        return cpu_affinity_;
    }
    inline std::shared_ptr<FeedOptions> opt() {
        return opt_;
    }

 private:
    static Config* instance_;
    std::shared_ptr<FeedOptions> opt_;

    string tcp_server_host_;
    int tcp_server_port_ = 0;
    string tcp_username_;
    string tcp_password_;

    string server_host_;
    int server_port_ = 0;
    string server_host_1_;
    int server_port_1_ = 0;
    string username_;
    string password_;
    int api_log_flag_ = 0;

    string interface_ip_;
    int cpu_affinity_ = 0;
};
}  // namespace co
