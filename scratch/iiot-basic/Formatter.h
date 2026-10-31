#pragma once

#include "ns3/internet-module.h"
#include <format>
#include <sstream>
#include <string>

template <>
struct std::formatter<ns3::Ipv4Address> : std::formatter<std::string> {
  auto format(const ns3::Ipv4Address &address, auto &ctx) const {
    std::ostringstream oss;
    oss << address;

    return std::formatter<std::string>::format(oss.str(), ctx);
  }
};
