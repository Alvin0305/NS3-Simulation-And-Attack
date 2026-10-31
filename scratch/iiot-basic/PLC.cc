#include "PLC.h"
#include "Coap.h"
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
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

  std::cout << "PLC: CoAP server started on UDP port " << Coap::PORT
            << std::endl;
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
      std::cout << "PLC: Invalid CoAP request" << std::endl;
      continue;
    }

    std::cout << std::fixed << std::setprecision(3) << "["
              << Simulator::Now().GetSeconds()
              << " s] PLC: Received CoAP request" << " path=/" << request.path
              << " code=" << static_cast<int>(request.code)
              << " MID=" << request.messageId << std::endl;

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
      std::cout << "PLC: Invalid temperature payload" << std::endl;
      Coap::Message response = Coap::MakeResponse(request, Coap::NOT_FOUND);
      Coap::Send(socket, senderAddress, response);
      continue;
    }

    Coap::Message response = Coap::MakeResponse(request, Coap::CHANGED);
    Coap::Send(socket, senderAddress, response);

    const bool coolerOn = temperature > threshold;
    const std::string command = coolerOn ? "COOLER ON" : "COOLER OFF";

    std::cout << std::fixed << std::setprecision(1)
              << "PLC: Temperature=" << temperature
              << " C, threshold=" << threshold << " C" << std::endl;
    std::cout << "PLC: Decision=" << command << std::endl;

    Coap::Message actuatorRequest =
        Coap::MakePost(nextMessageId++, "command", command);

    const Address actuatorDestination =
        InetSocketAddress(actuatorAddress, Coap::PORT);

    Coap::Send(clientSocket, actuatorDestination, actuatorRequest);

    std::cout << "PLC: CoAP POST /command" << " payload=" << command
              << " MID=" << actuatorRequest.messageId << std::endl;
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
      std::cout << "PLC: Invalid actuator response" << std::endl;
      continue;
    }

    std::cout << std::fixed << std::setprecision(3) << "["
              << Simulator::Now().GetSeconds() << " s] PLC: Actuator response"
              << " code=" << static_cast<int>(response.code)
              << " MID=" << response.messageId << std::endl;
  }
}
