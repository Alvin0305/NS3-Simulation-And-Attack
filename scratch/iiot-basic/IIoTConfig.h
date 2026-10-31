#pragma once

#include <ostream>
struct IIoTConfig {
  bool doDirectCommandInjection;
  bool sendSpoofedPacket;
  bool sendInvalidHMAC;
  bool sendReplayAttack;

  friend inline std::ostream &operator<<(std::ostream &out,
                                         const IIoTConfig &config) {
    out << "{ doDirectCommandInjection: " << config.doDirectCommandInjection
        << ", sendSpoofedPacket: " << config.sendSpoofedPacket
        << ", sendInvalidHMAC: " << config.sendInvalidHMAC
        << ", sendReplayAttack: " << config.sendReplayAttack << " }";
    return out;
  }
};
