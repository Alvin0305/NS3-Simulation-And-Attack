#include "Attacker.h"
#include "Coap.h"

#include "Logger.h"
#include "ns3/inet-socket-address.h"
#include "ns3/simulator.h"
#include "ns3/udp-socket-factory.h"

#include <cstdint>
#include <iostream>
#include <vector>

namespace ns3 {
NS_OBJECT_ENSURE_REGISTERED(Attacker);

TypeId Attacker::GetTypeId() {
  static TypeId id = TypeId("ns3::Attacker")
                         .SetParent<Application>()
                         .SetGroupName("Application")
                         .AddConstructor<Attacker>();

  return id;
}

Attacker::Attacker() : targetAddress(Ipv4Address::GetAny()), messageId(500){};

Attacker::~Attacker() = default;

void Attacker::SetTarget(Ipv4Address address) { targetAddress = address; }

void Attacker::StartApplication() {
  socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
  socket->Bind();
  socket->SetRecvCallback(MakeCallback(&Attacker::HandleRead, this));

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
  }
}

}; // namespace ns3
