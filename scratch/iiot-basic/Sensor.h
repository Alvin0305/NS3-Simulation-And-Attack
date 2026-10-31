#pragma once

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

#include <cstdint>

using namespace ns3;

class Sensor : public Application {
public:
  void SetDestination(Ipv4Address address, uint16_t port);

  void SetInterval(Time interval);

private:
  void StartApplication() override;
  void StopApplication() override;
  void SendReading();

  Ptr<Socket> socket;

  Ipv4Address destinationAddress;
  uint16_t destinationPort = 4000;

  Time interval = Seconds(1.0);

  uint32_t sequence = 1;
  EventId sendEvent;
};
