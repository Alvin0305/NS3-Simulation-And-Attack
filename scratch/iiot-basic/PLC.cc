#include "PLC.h"
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string>

void PLC::SetActuator(Ipv4Address address, uint16_t port) {
  this->actuatorAddress = address;
  this->actuatorPort = port;
}

void PLC::SetThreshold(double threshold) { this->threshold = threshold; }

void PLC::StartApplication() {
  receiveSocket =
      Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());

  receiveSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), 4000));
  receiveSocket->SetRecvCallback(MakeCallback(&PLC::HandleRead, this));

  sendSocket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
  sendSocket->Bind();
}

void PLC::StopApplication() {
  if (receiveSocket) {
    receiveSocket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    receiveSocket->Close();
    receiveSocket = nullptr;
  }

  if (sendSocket) {
    sendSocket->Close();
    sendSocket = nullptr;
  }
}

void PLC::HandleRead(Ptr<Socket> socket) {
  Address senderAddress;
  Ptr<Packet> packet;

  while ((packet = socket->RecvFrom(senderAddress))) {
    std::string message(packet->GetSize(), '\0');

    packet->CopyData(reinterpret_cast<uint8_t *>(&message[0]),
                     static_cast<uint32_t>(message.size()));

    std::cout << std::fixed << std::setprecision(3) << "["
              << Simulator::Now().GetSeconds() << " s] PLC: Received "
              << message << std::endl;

    const std::string key = "temperature=";
    const std::size_t position = message.find(key);

    if (position == std::string::npos) {
      std::cout << "PLC: Invalid temperature message" << std::endl;
      continue;
    }

    double temperature;

    try {
      temperature = std::stod(message.substr(position + key.size()));
    } catch (const std::exception &e) {
      std::cout << "PLC: Could not parse temperature" << std::endl;
      continue;
    }

    const bool coolerOn = temperature > threshold;
    const std::string command = coolerOn ? "COOLER ON" : "COOLER OFF";

    std::cout << std::fixed << std::setprecision(1)
              << "PLC: Temperature=" << temperature
              << " C, threshold=" << threshold << " C" << std::endl;
    std::cout << "PLC: Decision=" << command << std::endl;

    Ptr<Packet> commandPacket =
        Create<Packet>(reinterpret_cast<const uint8_t *>(command.data()),
                       static_cast<uint32_t>(command.size()));

    sendSocket->SendTo(commandPacket, 0,
                       InetSocketAddress(actuatorAddress, actuatorPort));
  }
}
