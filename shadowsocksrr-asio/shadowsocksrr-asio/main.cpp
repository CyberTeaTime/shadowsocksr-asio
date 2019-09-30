﻿/*
 *  This file is part of the shadowsocksrr-asio project.
 *  shadowsocksrr-asio is a shadowsocksr implement which driven by boost.asio.
 *  Copyright (C) 2018  Akkariiin
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <memory>
#include <atomic>
#include <csignal>

#include <boost/program_options.hpp>

#include "log.h"
#include "version.h"

#include "service.h"
;

static std::atomic_bool restart(false);

std::weak_ptr<Service> service_weak_ref;


void handleTermination(int) {
    Log::log_with_date_time("shadowsocksrr-asio service handled Termination sign.", Log::Level::FATAL);
    auto ptr = service_weak_ref.lock();
    if (ptr)
        ptr->stop();
}

void restartService(int) {
    Log::log_with_date_time("shadowsocksrr-asio service handled Restart sign.", Log::Level::FATAL);
    restart = true;
    auto ptr = service_weak_ref.lock();
    if (ptr)
        ptr->stop();
}

int main(int argc, const char *const argv[]) {
    Log::log_with_date_time("Welcome to shadowsocksrr-asio " + Version::get_version(), Log::Level::FATAL);

    std::string configFilePath{};
    std::string configJsonString{};
    {
        namespace po = boost::program_options;

        po::options_description desc{"hadowsocksrr-asio"};
        desc.add_options()
                ("help", "produce help message")
                ("configFile", po::value<std::string>(), "config file path")
                ("configJson", po::value<std::string>(), "config Json string")
            /* TODO other */;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);


        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 1;
        }

        if (vm.count("configFile")) {
            configFilePath = vm["configFile"].as<std::string>();
        }
        if (vm.count("configJson")) {
            configJsonString = vm["configJson"].as<std::string>();
        }
    }

    std::shared_ptr<MainConfig> config_ = std::make_shared<MainConfig>();

    signal(SIGINT, handleTermination);
    signal(SIGTERM, handleTermination);
#ifndef _WIN32
    signal(SIGHUP, restartService);
#endif // _WIN32

    try {
        // load from file or command
        if (configFilePath.empty()) {
            config_->loadJsonString(configJsonString);
        } else {
            config_->loadFile(configFilePath);
        }

        do {
            restart = false;
            {
                std::shared_ptr<Service> service = std::make_shared<Service>(config_);
                service_weak_ref = service->weak_from_this();
                service->run();
            }
            if (restart) {
                Log::log_with_date_time("shadowsocksrr-asio service now restarting. . . ", Log::Level::FATAL);
            }
        } while (restart);
        system("pause");
    }
    catch (const std::exception &e) {
        Log::log_with_date_time(std::string("fatal: ") + e.what(), Log::Level::FATAL);
        Log::log_with_date_time("exiting. . . ", Log::Level::FATAL);
        // exit(1);
        system("pause");
        return 1;
    }
    return 0;
}
