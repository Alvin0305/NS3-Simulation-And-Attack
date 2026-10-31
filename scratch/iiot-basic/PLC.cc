#include "PLC.h"
#include "Auth.h"
#include "Coap.h"
#include "Logger.h"
#include <cstddef>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

void PLC::SetActuator(Ipv4Address address) { this->actuatorAddress = address; }

void PLC::SetThreshold(double threshold) { this->threshold = threshold; }

void PLC::StartApplication() {
  receiveSocket =
      Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());

  receiveSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), Coap::PORT));
  receiveSocket->SetRecvCallback(
      MakeCallback(&PLC::HandleTemperatureRequest, this));

  clientSocket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
  clientSocket->Bind();

  clientSocket->SetRecvCallback(
      MakeCallback(&PLC::HandleActuatorResponse, this));

  PLC_LOG_INFO("CoAP Server started on UDP port: {}", Coap::PORT);
}

void PLC::StopApplication() {
  if (receiveSocket) {
    receiveSocket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    receiveSocket->Close();
    receiveSocket = nullptr;
  }

  if (clientSocket) {
    clientSocket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    clientSocket->Close();
    clientSocket = nullptr;
  }
}

void PLC::HandleTemperatureRequest(Ptr<Socket> socket) {
  Address senderAddress;
  Ptr<Packet> packet;

  while ((packet = socket->RecvFrom(senderAddress))) {
    std::vector<uint8_t> bytes(packet->GetSize());

    packet->CopyData(bytes.data(), static_cast<uint32_t>(bytes.size()));

    Coap::Message request;

    if (not Coap::Decode(bytes.data(), bytes.size(), request)) {
      PLC_LOG_ERROR("Invalid CoAP request");
      continue;
    }

    PLC_LOG_INFO("Received CoAP request path=/{} code={}, MID={}", request.path,
                 static_cast<int>(request.code), request.messageId);

    if (request.type != Coap::CON || request.code != Coap::POST ||
        request.path != "temperature") {
      Coap::Message response = Coap::MakeResponse(request, Coap::NOT_FOUND);
      Coap::Send(socket, senderAddress, response);
      continue;
    }

    double temperature;

    try {
      std::size_t parsedCharacters = 0;
      temperature = std::stod(request.payload, &parsedCharacters);

      if (parsedCharacters != request.payload.size()) {
        throw std::invalid_argument("Extra characters in temperature");
      }
    } catch (const std::exception &e) {
      PLC_LOG_ERROR("Invalid Temperature payload");
      Coap::Message response = Coap::MakeResponse(request, Coap::NOT_FOUND);
      Coap::Send(socket, senderAddress, response);
      continue;
    }

    Coap::Message response = Coap::MakeResponse(request, Coap::CHANGED);
    Coap::Send(socket, senderAddress, response);

    const bool coolerOn = temperature > threshold;
    const std::string command = coolerOn ? "COOLER ON" : "COOLER OFF";

    PLC_LOG_INFO("Temperature={} C, threshold={} C, Decision={}", temperature,
                 threshold, command);

    const uint64_t sequence = nextCommandSequence++;
    const std::string canonicalMessage =
        std::string("/command\n") + command + "\n" + std::to_string(sequence);
    const std::string tag = Auth::ComputeTag(canonicalMessage);

    if (tag.empty()) {
      PLC_LOG_ERROR("Failed to generate HMAC. Command not sent");
      continue;
    }

    const std::string authenticatedPayload =
        command + "|" + std::to_string(sequence) + "|" + tag;

    Coap::Message actuatorRequest =
        Coap::MakePost(nextMessageId++, "command", authenticatedPayload);

    const Address actuatorDestination =
        InetSocketAddress(actuatorAddress, Coap::PORT);

    int result = Coap::Send(clientSocket, actuatorDestination, actuatorRequest);

    if (result < 0) {
      PLC_LOG_ERROR("Failed to send authenticated actuator command");
    } else {
      PLC_LOG_INFO("CoAP POST /command payload={}, sequence={}, MID={}",
                   command, sequence, actuatorRequest.messageId);
    }
  }
}

void PLC::HandleActuatorResponse(Ptr<Socket> socket) {
  Address senderAddress;
  Ptr<Packet> packet;

  while ((packet = socket->RecvFrom(senderAddress))) {
    std::vector<uint8_t> bytes(packet->GetSize());
    packet->CopyData(bytes.data(), static_cast<uint32_t>(bytes.size()));

    Coap::Message response;

    if (not Coap::Decode(bytes.data(), bytes.size(), response)) {
      PLC_LOG_ERROR("Invalid actuator response");
      continue;
    }

    PLC_LOG_INFO("Actuator response code={}, MID={}",
                 static_cast<int>(response.code), response.messageId);
  }
}
