#pragma once

#include "cooler/Cooler.h"
#include "utils/IIoTConfig.h"

#include "ns3/internet-module.h"

#include <format>
#include <sstream>
#include <string>

template <>
struct std::formatter<ns3::Ipv4Address> : std::formatter<std::string> {
    auto format(const ns3::Ipv4Address& address, auto& ctx) const {
        std::ostringstream oss;
        oss << address;

        return std::formatter<std::string>::format(oss.str(), ctx);
    }
};

template <>
struct std::formatter<Cooler::Status> : std::formatter<std::string> {
    auto format(const Cooler::Status& status, auto& ctx) const {
        std::ostringstream oss;
        oss << status;

        return std::formatter<std::string>::format(oss.str(), ctx);
    }
};

template <>
struct std::formatter<IIoTConfig> : std::formatter<std::string> {
    auto format(const IIoTConfig& config, auto& ctx) const {
        std::ostringstream oss;
        oss << config;

        return std::formatter<std::string>::format(oss.str(), ctx);
    }
};
