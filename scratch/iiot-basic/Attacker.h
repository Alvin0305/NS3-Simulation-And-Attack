#pragma once

#include "IIoTConfig.h"
#include "ns3/application.h"
#include "ns3/inet-socket-address.h"
#include "ns3/network-module.h"
#include <cstdint>

namespace ns3 {
class Attacker : public Application {
public:
  static TypeId GetTypeId();

  const IIoTConfig &config;

  Attacker(const IIoTConfig &config);
  ~Attacker() override;

  void SetTarget(Ipv4Address address);

private:
  void StartApplication() override;
  void StopApplication() override;

  void HandleRead(Ptr<Socket> socket);

  void SendSpoofedCommand();

  Ptr<Socket> socket;
  Ptr<Socket> rawSocket;

  Ipv4Address targetAddress;
  uint16_t messageId;
};

}; // namespace ns3
