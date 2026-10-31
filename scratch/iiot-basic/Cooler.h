#pragma once

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

class Cooler : public Application {
  enum Status { ON, OFF };

private:
  void StartApplication() override;
  void StopApplication() override;

  void HandleRead(Ptr<Socket> socket);

  Ptr<Socket> socket;
  Status status = OFF;
};
