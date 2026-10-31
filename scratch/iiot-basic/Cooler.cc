#include "Cooler.h"
#include "Coap.h"
#include "Logger.h"
#include <cstdint>
#include <fcntl.h>

void Cooler::StartApplication() {
  socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
  socket->Bind(InetSocketAddress(Ipv4Address::GetAny(), Coap::PORT));
  socket->SetRecvCallback(MakeCallback(&Cooler::HandleRead, this));

  COOLER_LOG_INFO("CoAP server started on UDP port: {}", Coap::PORT);
}

void Cooler::StopApplication() {
  if (socket) {
    socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    socket->Close();
    socket = nullptr;
  }
}

void Cooler::HandleRead(Ptr<Socket> socket) {
  Address senderAddress;
  Ptr<Packet> packet;

  while ((packet = socket->RecvFrom(senderAddress))) {
    std::vector<uint8_t> bytes(packet->GetSize());

    packet->CopyData(bytes.data(), static_cast<uint32_t>(bytes.size()));

    Coap::Message request;

    if (not Coap::Decode(bytes.data(), bytes.size(), request)) {
      COOLER_LOG_ERROR("Invalid CoAP request");
      continue;
    }

    Ipv4Address senderIP = Ipv4Address::GetAny();
    if (InetSocketAddress::IsMatchingType(senderAddress)) {
      senderIP = InetSocketAddress::ConvertFrom(senderAddress).GetIpv4();
    }

    COOLER_LOG_INFO("Received CoAP request from={}, path=/{}, MID={}", senderIP,
                    request.path, request.messageId);

    uint8_t responseCode = Coap::NOT_FOUND;

    if (request.type == Coap::CON && request.code == Coap::POST &&
        request.path == "command") {
      if (request.payload == "COOLER ON") {
        COOLER_LOG_INFO("Cooler state=ON");
        status = ON;
        responseCode = Coap::CHANGED;
      } else if (request.payload == "COOLER OFF") {
        COOLER_LOG_INFO("Cooler state=OFF");
        status = OFF;
        responseCode = Coap::CHANGED;
      } else {
        COOLER_LOG_ERROR("Unknown command: {}", request.payload);
      }

      Coap::Message response = Coap::MakeResponse(request, responseCode);
      Coap::Send(socket, senderAddress, response);
    }
  }
}
