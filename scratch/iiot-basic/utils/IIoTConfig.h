#pragma once

#include <ostream>

struct IIoTConfig {
    bool doDirectCommandInjection;
    bool sendSpoofedPacket;
    bool sendInvalidHMAC;
    bool sendReplayAttack;

    bool defendDirectCommandInjection;
    bool defendIPSpoofing;
    bool defendReplayAttack;

    friend inline std::ostream& operator<<(std::ostream& out, const IIoTConfig& config) {
        out << "{ doDirectCommandInjection: " << config.doDirectCommandInjection
            << ", sendSpoofedPacket: " << config.sendSpoofedPacket << ", sendInvalidHMAC: " << config.sendInvalidHMAC
            << ", sendReplayAttack: " << config.sendReplayAttack << ", defendDirectCommandInjection"
            << config.defendDirectCommandInjection << ", defendIPSpoofing" << config.defendIPSpoofing << ", defendReplayAttack"
            << config.defendReplayAttack << " }";
        return out;
    }
};
