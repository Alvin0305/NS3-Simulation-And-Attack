#include "Cooler.h"
#include "Auth.h"
#include "Coap.h"
#include "Constants.h"
#include "Logger.h"
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <string>

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

  // Added source IP based authorization
  const Ipv4Address authorizedPLCAddress(constants::plcIP);

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

    const std::size_t firstSeparator = request.payload.find('|');
    if (firstSeparator == std::string::npos) {
      COOLER_LOG_WARN(
          "AUTHENTICATION FAILED: missing command metadata, source={}",
          senderIP);

      Coap::Message response = Coap::MakeResponse(request, Coap::FORBIDDEN);
      Coap::Send(socket, senderAddress, response);
      continue;
    }

    const std::size_t secondSeparator =
        request.payload.find('|', firstSeparator + 1);
    if (secondSeparator == std::string::npos ||
        request.payload.find('|', secondSeparator + 1) != std::string::npos) {
      COOLER_LOG_WARN(
          "AUTHENTICATION FAILED: malformed command payload, source={}",
          senderIP);

      Coap::Message response = Coap::MakeResponse(request, Coap::FORBIDDEN);
      Coap::Send(socket, senderAddress, response);
      continue;
    }

    const std::string command = request.payload.substr(0, firstSeparator);
    const std::string sequenceText = request.payload.substr(
        firstSeparator + 1, secondSeparator - firstSeparator - 1);
    const std::string receivedTag = request.payload.substr(secondSeparator + 1);

    uint64_t sequence = 0;
    try {
      std::size_t parsedCharacters = 0;
      sequence = std::stoull(sequenceText, &parsedCharacters);

      if (sequenceText.empty() ||
          sequenceText.find_first_not_of("0123456789") != std::string::npos ||
          parsedCharacters != sequenceText.size()) {
        throw std::invalid_argument("Invalid sequence number");
      }
    } catch (const std::exception &) {
      COOLER_LOG_WARN(
          "AUTHENTICATION FAILED: invalid sequence number, source={}",
          senderIP);

      Coap::Message response = Coap::MakeResponse(request, Coap::FORBIDDEN);
      Coap::Send(socket, senderAddress, response);
      continue;
    }

    const std::string canonicalMessage =
        std::string("/command\n") + command + "\n" + sequenceText;

    if (not Auth::VerifyTag(canonicalMessage, receivedTag)) {
      COOLER_LOG_WARN(
          "AUTHENTICATION FAILED: invalid HMAC, source={}, sequence={}",
          senderIP, sequence);

      Coap::Message response = Coap::MakeResponse(request, Coap::FORBIDDEN);
      Coap::Send(socket, senderAddress, response);
      continue;
    }

    COOLER_LOG_INFO("HMAC verification successful: source={}, sequence={}",
                    senderIP, sequence);

    if (hasAcceptedSequence && sequence <= lastAcceptedSequence) {
      COOLER_LOG_WARN(
          "REPLAY REJECTED: received sequence={}, last accepted={}, "
          "source={}",
          sequence, lastAcceptedSequence, senderIP);

      Coap::Message response = Coap::MakeResponse(request, Coap::FORBIDDEN);
      Coap::Send(socket, senderAddress, response);
      continue;
    }

    Status requestedStatus;

    if (command == "COOLER ON") {
      requestedStatus = ON;
    } else if (command == "COOLER OFF") {
      requestedStatus = OFF;
    } else {
      COOLER_LOG_ERROR("Unknown command: {}", command);
      Coap::Message response = Coap::MakeResponse(request, Coap::NOT_FOUND);
      Coap::Send(socket, senderAddress, response);
      continue;
    }

    lastAcceptedSequence = sequence;
    hasAcceptedSequence = true;

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
