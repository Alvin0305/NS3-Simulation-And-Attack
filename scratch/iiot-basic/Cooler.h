#pragma once

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include <cstdint>

using namespace ns3;

class Cooler : public Application {
public:
  void setPort(uint16_t port);

private:
  void StartApplication() override;
  void StopApplication() override;

  void HandleRead(Ptr<Socket> socket);

  Ptr<Socket> socket;
  uint16_t port = 5000;
};
