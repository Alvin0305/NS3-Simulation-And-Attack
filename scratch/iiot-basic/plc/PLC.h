#pragma once

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

#include <cstdint>

using namespace ns3;

class PLC : public Application {
  public:
    void SetCoolerAddress(Ipv4Address address);
    void SetThreshold(double threshold);

  private:
    void StartApplication() override;
    void StopApplication() override;

    void HandleTemperatureRequest(Ptr<Socket> socket);
    void HandleCoolerResponse(Ptr<Socket> socket);

    Ptr<Socket> recvSocketFromSensor;
    Ptr<Socket> sendSocketToCooler;

    Ipv4Address coolerAddress;

    double threshold = 70.0;
    uint16_t nextMessageId = 100;

    uint64_t nextCommandSequence = 1;
};
