#pragma once

#include <ostream>
struct IIoTConfig {
  bool doDirectCommandInjection;
  bool sendSpoofedPacket;

  friend inline std::ostream &operator<<(std::ostream &out,
                                         const IIoTConfig &config) {
    out << "{ doDirectCommandInjection: " << config.doDirectCommandInjection
        << ", sendSpoofedPacket: " << config.sendSpoofedPacket << " }";
    return out;
  }
};
