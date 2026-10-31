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

  // Added authorization
  const Ipv4Address authorizedPLCAddress("10.1.1.2");

  while ((packet = socket->RecvFrom(senderAddress))) {
    std::vector<uint8_t> bytes(packet->GetSize());

    packet->CopyData(bytes.data(), static_cast<uint32_t>(bytes.size()));

    Coap::Message request;

    if (not Coap::Decode(bytes.data(), bytes.size(), request)) {
      COOLER_LOG_ERROR("Invalid CoAP request");
      continue;
    }

    Ipv4Address senderIP = Ipv4Address::GetAny();
    if (not InetSocketAddress::IsMatchingType(senderAddress)) {
      COOLER_LOG_ERROR("Rejected request (extracting sender address failed)");
      continue;
    }

    senderIP = InetSocketAddress::ConvertFrom(senderAddress).GetIpv4();
    COOLER_LOG_INFO(
        "Received CoAP request from={}, path=/{}, MID={}, payload={}", senderIP,
        request.path, request.messageId, request.payload);

    if (request.type != Coap::CON || request.code != Coap::POST ||
        request.path != "command") {
      COOLER_LOG_WARN(
          "A non CONNECT or non POST or non COMMAND packet came. Rejecting it");

      Coap::Message response = Coap::MakeResponse(request, Coap::NOT_FOUND);
      Coap::Send(socket, senderAddress, response);
      continue;
    }

    if (senderIP != authorizedPLCAddress) {
      COOLER_LOG_WARN("ACCESS DENIED. source={}, authorized plc={} "
                      "reason=unauthorized sender",
                      senderIP, authorizedPLCAddress);

      Coap::Message response = Coap::MakeResponse(request, Coap::FORBIDDEN);
      Coap::Send(socket, senderAddress, response);
      continue;
    }

    Status requestedStatus;

    if (request.payload == "COOLER ON") {
      requestedStatus = ON;
    } else if (request.payload == "COOLER OFF") {
      requestedStatus = OFF;
    } else {
      COOLER_LOG_ERROR("Unknown command: {}", request.payload);
      Coap::Message response = Coap::MakeResponse(request, Coap::NOT_FOUND);
      Coap::Send(socket, senderAddress, response);
      continue;
    }

    if (status != requestedStatus) {
      COOLER_LOG_INFO("Status changed: {} -> {}, source={}", status,
                      requestedStatus, senderIP);
      status = requestedStatus;
    } else {
      COOLER_LOG_INFO("Status unchanged: {}, source={}", status, senderIP);
    }

    Coap::Message response = Coap::MakeResponse(request, Coap::CHANGED);
    Coap::Send(socket, senderAddress, response);
  }
}
