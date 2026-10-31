#pragma once

#include "IIoTConfig.h"
#include "ns3/application.h"
#include "ns3/inet-socket-address.h"
#include "ns3/network-module.h"
#include <cstdint>
#include <string>
#include <vector>

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

  bool PromiscSniffer(Ptr<NetDevice> device, Ptr<const Packet> packet,
                      uint16_t protocol, const Address &from, const Address &to,
                      NetDevice::PacketType packetType);
  void SendConfiguredAttacks();
  void SendInvalidHmacAttack();
  void SendReplayAttack();

  void SendSpoofedCoapBytes(const std::vector<uint8_t> &coapBytes, uint16_t mid,
                            const std::string &attackName);

  Ptr<Socket> socket;
  Ptr<Socket> rawSocket;

  Ipv4Address targetAddress;
  uint16_t messageId;

  std::vector<uint8_t> capturedCoapBytes;
  uint16_t capturedMessageId = 0;
  bool hasCapturedCommand = false;
};

}; // namespace ns3
