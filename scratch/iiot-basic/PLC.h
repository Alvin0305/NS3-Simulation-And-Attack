#pragma once

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include <cstdint>

using namespace ns3;

class PLC : public Application {
public:
  void SetActuator(Ipv4Address address, uint16_t port);
  void SetThreshold(double threshold);

private:
  void StartApplication() override;
  void StopApplication() override;

  void HandleRead(Ptr<Socket> socket);

  Ptr<Socket> receiveSocket;
  Ptr<Socket> sendSocket;

  Ipv4Address actuatorAddress;
  uint16_t actuatorPort = 5000;

  double threshold = 70.0;
};
