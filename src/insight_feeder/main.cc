// Copyright 2025 Fancapital Inc.  All rights reserved.
#include <boost/program_options.hpp>
#include "config.h"
#include "insight_server.h"

using namespace std;
using namespace co;
namespace po = boost::program_options;

constexpr const char* kVersion = "v2.0.21";

int main(int argc, char* argv[]) {
    try {
        po::options_description desc("[insight_feeder] Usage");
        desc.add_options()
            ("passwd", po::value<std::string>(), "encode plain password")
            ("help,h", "show help message")
            ("version,v", "show version information");
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
        if (vm.count("passwd")) {
            cout << co::EncodePassword(vm["passwd"].as<std::string>()) << endl;
            return 0;
        } else if (vm.count("help")) {
            cout << desc << endl;
            return 0;
        } else if (vm.count("version")) {
            cout << kVersion << endl;
            return 0;
        }
        LOG_INFO << "start insight udp feeder: version = " << kVersion << " ...";
        auto opt = Config::Load();
        InsightServer server;
        server.Init(opt);
        server.Run();
        LOG_INFO << "server is stopped.";
    } catch (std::exception& e) {
        LOG_FATAL << "server is crashed, " << e.what();
        return 2;
    } catch (...) {
        LOG_FATAL << "server is crashed, unknown reason";
        return 3;
    }
    return 0;
}

