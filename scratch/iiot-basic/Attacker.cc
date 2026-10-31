#include "Attacker.h"
#include "Coap.h"

#include "Constants.h"
#include "IIoTConfig.h"
#include "Logger.h"
#include "ns3/boolean.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-raw-socket-factory.h"
#include "ns3/simulator.h"
#include "ns3/udp-header.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/uinteger.h"

#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <vector>

namespace ns3 {
NS_OBJECT_ENSURE_REGISTERED(Attacker);

TypeId Attacker::GetTypeId() {
  static TypeId id = TypeId("ns3::Attacker")
                         .SetParent<Application>()
                         .SetGroupName("Application");

  return id;
}

Attacker::Attacker(const IIoTConfig &config)
    : config(config), targetAddress(Ipv4Address::GetAny()), messageId(500){};

Attacker::~Attacker() = default;

void Attacker::SetTarget(Ipv4Address address) { targetAddress = address; }

void Attacker::StartApplication() {
  socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
  socket->Bind();
  socket->SetRecvCallback(MakeCallback(&Attacker::HandleRead, this));

  if (config.doDirectCommandInjection) {
    ATTACKER_LOG_INFO("Sending unauthorized command");
    Coap::Message request = Coap::MakePost(messageId, "command", "COOLER OFF");
    const Address destination = InetSocketAddress(targetAddress, Coap::PORT);
    const int result = Coap::Send(socket, destination, request);

    if (result < 0) {
      ATTACKER_LOG_ERROR("Failed to send command");
    } else {
      ATTACKER_LOG_INFO("Send POST /command payload=COOLER OFF MID={}",
                        messageId);
    }
    messageId++;
  }

  if (config.sendSpoofedPacket) {
    ATTACKER_LOG_INFO("Sending Spoofed command");
    SendSpoofedCommand();
    messageId++;
  }
}

void Attacker::StopApplication() {
  if (socket) {
    socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    socket->Close();
    socket = nullptr;
  }
}

void Attacker::HandleRead(Ptr<Socket> socket) {
  Address senderAddress;
  Ptr<Packet> packet;

  while ((packet = socket->RecvFrom(senderAddress))) {
    std::vector<uint8_t> bytes(packet->GetSize());
    packet->CopyData(bytes.data(), static_cast<uint32_t>(bytes.size()));

    Coap::Message response;
    if (not Coap::Decode(bytes.data(), bytes.size(), response)) {
      ATTACKER_LOG_ERROR("Received an invalid CoAP response");
      std::cout << "ATTACKER: Received an invalid CoAP response" << std::endl;
      continue;
    }

    ATTACKER_LOG_INFO("Received response code={}, MID={}",
                      static_cast<unsigned>(response.code), response.messageId);

    switch (response.code) {
    case Coap::CHANGED:
      ATTACKER_LOG_INFO("Command accepted by the cooler. MID={}",
                        response.messageId);
      break;

    case Coap::FORBIDDEN:
      ATTACKER_LOG_WARN(
          "Command REJECTED by the cooler: unauthorized sender. MID={}",
          response.messageId);
      break;

    case Coap::NOT_FOUND:
      ATTACKER_LOG_WARN("Command failed: resource not found. MID={}",
                        response.messageId);
      break;

    default:
      ATTACKER_LOG_WARN(
          "Received an unexpected CoAP response code={} for MID={}",
          static_cast<unsigned>(response.code), response.messageId);
      break;
    }
  }
}
} // namespace ns3

void Attacker::SendSpoofedCommand() {
  const Ipv4Address forgedSource(constants::plcIP);
  const Ipv4Address destination(constants::coolerIP);

  Coap::Message command = Coap::MakePost(messageId, constants::commandString,
                                         constants::coolerOffCommand);
  std::vector<uint8_t> coapBytes = Coap::Encode(command);

  Ptr<Packet> packet =
      Create<Packet>(coapBytes.data(), static_cast<uint32_t>(coapBytes.size()));

  UdpHeader udp;
  udp.SetSourcePort(constants::attackerPort);
  udp.SetDestinationPort(Coap::PORT);
  udp.EnableChecksums();
  udp.InitializeChecksum(forgedSource, destination, constants::protocol::udp);

  packet->AddHeader(udp);

  Ipv4Header ip;
  ip.SetSource(forgedSource);
  ip.SetDestination(destination);
  ip.SetProtocol(constants::protocol::udp);
  ip.SetTtl(64);
  ip.SetIdentification(messageId);
  ip.SetPayloadSize(packet->GetSize());

  packet->AddHeader(ip);

  rawSocket =
      Socket::CreateSocket(GetNode(), Ipv4RawSocketFactory::GetTypeId());
  rawSocket->SetAttribute("Protocol", UintegerValue(constants::protocol::udp));
  rawSocket->SetAttribute("IpHeaderInclude", BooleanValue(true));

  Address target = InetSocketAddress(destination, 0);
  int result = rawSocket->SendTo(packet, 0, target);

  if (result < 0) {
    ATTACKER_LOG_ERROR("Failed to send spoofed command");
  } else {
    ATTACKER_LOG_INFO("Sent spoofed CoAP POST /command payload={}, "
                      "forged_source={}, destination={}, MID={}",
                      constants::coolerOffCommand, constants::plcIP,
                      constants::coolerIP, messageId);
  }
}

; // namespace ns3
