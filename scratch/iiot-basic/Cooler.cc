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

  const Ipv4Address authorizedPLCAddress(constants::plcIP);

  while ((packet = socket->RecvFrom(senderAddress))) {
    std::vector<uint8_t> bytes(packet->GetSize());

    packet->CopyData(bytes.data(), static_cast<uint32_t>(bytes.size()));

    Coap::Message request;

    if (!Coap::Decode(bytes.data(), bytes.size(), request)) {
      COOLER_LOG_ERROR("Invalid CoAP request");
      continue;
    }

    if (!InetSocketAddress::IsMatchingType(senderAddress)) {
      COOLER_LOG_ERROR("Rejected request: extracting sender address failed");
      continue;
    }

    const Ipv4Address senderIP =
        InetSocketAddress::ConvertFrom(senderAddress).GetIpv4();

    COOLER_LOG_INFO(
        "Received CoAP request from={}, path=/{}, MID={}, payload={}", senderIP,
        request.path, request.messageId, request.payload);

    if (request.type != Coap::CON || request.code != Coap::POST ||
        request.path != constants::commandString) {

      COOLER_LOG_WARN("Invalid request type, method, or command path");

      Coap::Message response = Coap::MakeResponse(request, Coap::NOT_FOUND);

      Coap::Send(socket, senderAddress, response);
      continue;
    }

    if (config.defendIPSpoofing && senderIP != authorizedPLCAddress) {

      COOLER_LOG_WARN("ACCESS DENIED: source={}, authorized PLC={}, "
                      "reason=source IP not allowed",
                      senderIP, authorizedPLCAddress);

      Coap::Message response = Coap::MakeResponse(request, Coap::FORBIDDEN);

      Coap::Send(socket, senderAddress, response);
      continue;
    }

    if (!config.defendIPSpoofing) {
      COOLER_LOG_INFO("IP allowlist defense disabled by configuration");
    }

    std::string command;
    std::string sequenceText;
    std::string receivedTag;

    uint64_t sequence = 0;
    bool hasMetadata = false;

    const std::size_t firstSeparator = request.payload.find('|');

    if (firstSeparator == std::string::npos) {
      command = request.payload;

      if (config.defendDirectCommandInjection) {
        COOLER_LOG_WARN("AUTHENTICATION FAILED: missing command metadata, "
                        "source={}",
                        senderIP);

        Coap::Message response = Coap::MakeResponse(request, Coap::FORBIDDEN);

        Coap::Send(socket, senderAddress, response);
        continue;
      }

      if (config.defendReplayAttack) {
        COOLER_LOG_WARN("REPLAY PROTECTION FAILED: missing sequence metadata, "
                        "source={}",
                        senderIP);

        Coap::Message response = Coap::MakeResponse(request, Coap::FORBIDDEN);

        Coap::Send(socket, senderAddress, response);
        continue;
      }

      COOLER_LOG_WARN("Accepting unauthenticated command format because "
                      "authentication and replay defenses are disabled");

    } else {
      const std::size_t secondSeparator =
          request.payload.find('|', firstSeparator + 1);

      if (secondSeparator == std::string::npos ||
          request.payload.find('|', secondSeparator + 1) != std::string::npos) {

        COOLER_LOG_WARN("Rejected malformed command metadata, source={}",
                        senderIP);

        Coap::Message response = Coap::MakeResponse(request, Coap::FORBIDDEN);

        Coap::Send(socket, senderAddress, response);
        continue;
      }

      command = request.payload.substr(0, firstSeparator);

      sequenceText = request.payload.substr(
          firstSeparator + 1, secondSeparator - firstSeparator - 1);

      receivedTag = request.payload.substr(secondSeparator + 1);

      try {
        std::size_t parsedCharacters = 0;

        if (sequenceText.empty() ||
            sequenceText.find_first_not_of("0123456789") != std::string::npos) {
          throw std::invalid_argument("Invalid sequence number");
        }

        sequence = std::stoull(sequenceText, &parsedCharacters);

        if (parsedCharacters != sequenceText.size()) {
          throw std::invalid_argument("Invalid sequence number");
        }

      } catch (const std::exception &) {
        COOLER_LOG_WARN("Rejected invalid sequence number, source={}",
                        senderIP);

        Coap::Message response = Coap::MakeResponse(request, Coap::FORBIDDEN);

        Coap::Send(socket, senderAddress, response);
        continue;
      }

      hasMetadata = true;
    }

    if (config.defendDirectCommandInjection) {
      if (!hasMetadata || receivedTag.size() != 64) {
        COOLER_LOG_WARN("AUTHENTICATION FAILED: missing or malformed HMAC, "
                        "source={}",
                        senderIP);

        Coap::Message response = Coap::MakeResponse(request, Coap::FORBIDDEN);

        Coap::Send(socket, senderAddress, response);
        continue;
      }

      const std::string canonicalMessage =
          std::string("/command\n") + command + "\n" + sequenceText;

      if (!Auth::VerifyTag(canonicalMessage, receivedTag)) {
        COOLER_LOG_WARN("AUTHENTICATION FAILED: invalid HMAC, "
                        "source={}, sequence={}",
                        senderIP, sequence);

        Coap::Message response = Coap::MakeResponse(request, Coap::FORBIDDEN);

        Coap::Send(socket, senderAddress, response);
        continue;
      }

      COOLER_LOG_INFO("HMAC verification successful: source={}, sequence={}",
                      senderIP, sequence);

    } else {
      COOLER_LOG_WARN("HMAC authentication defense disabled by configuration");
    }

    if (config.defendReplayAttack) {
      if (!hasMetadata) {
        COOLER_LOG_WARN("REPLAY PROTECTION FAILED: sequence unavailable");

        Coap::Message response = Coap::MakeResponse(request, Coap::FORBIDDEN);

        Coap::Send(socket, senderAddress, response);
        continue;
      }

      if (hasAcceptedSequence && sequence <= lastAcceptedSequence) {

        COOLER_LOG_WARN("REPLAY REJECTED: received sequence={}, "
                        "last accepted={}, source={}",
                        sequence, lastAcceptedSequence, senderIP);

        Coap::Message response = Coap::MakeResponse(request, Coap::FORBIDDEN);

        Coap::Send(socket, senderAddress, response);
        continue;
      }

    } else {
      COOLER_LOG_WARN("Replay protection disabled by configuration");
    }

    Status requestedStatus;

    if (command == constants::coolerOnCommand) {
      requestedStatus = ON;

    } else if (command == constants::coolerOffCommand) {
      requestedStatus = OFF;

    } else {
      COOLER_LOG_ERROR("Unknown command: {}", command);

      Coap::Message response = Coap::MakeResponse(request, Coap::NOT_FOUND);

      Coap::Send(socket, senderAddress, response);
      continue;
    }

    if (config.defendReplayAttack) {
      lastAcceptedSequence = sequence;
      hasAcceptedSequence = true;
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
