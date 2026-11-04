#pragma once

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

#include <cstdint>

using namespace ns3;

class Sensor : public Application {
  public:
    void SetDestination(Ipv4Address address);

  private:
    void StartApplication() override;
    void StopApplication() override;

    void SendReading();
    void HandleResponse(Ptr<Socket> socket);

    Ptr<Socket> socket;

    Ipv4Address destinationAddress;

    uint32_t sequence = 1;
    uint16_t nextMessageId = 1;
    EventId sendEvent;
};
