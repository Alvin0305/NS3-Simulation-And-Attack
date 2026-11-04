#pragma once

#include "utils/IIoTConfig.h"

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

#include <ostream>

using namespace ns3;

class Cooler : public Application {
  public:
    Cooler(const IIoTConfig& config)
        : config(config) {};

    enum Status {
        ON,
        OFF
    };

    friend inline std::ostream& operator<<(std::ostream& out, const Status& status) {
        switch (status) {
            case ON:
                out << "ON";
                break;
            case OFF:
                out << "OFF";
                break;
            default:
                out << "Unknown";
                break;
        }

        return out;
    }

  private:
    void StartApplication() override;
    void StopApplication() override;

    void HandleRequest(Ptr<Socket> socket);

    const IIoTConfig& config;

    Ptr<Socket> socketToPLC;
    Status status = OFF;

    uint64_t lastAcceptedSequence = 0;
    bool hasAcceptedSequence = false;
};
