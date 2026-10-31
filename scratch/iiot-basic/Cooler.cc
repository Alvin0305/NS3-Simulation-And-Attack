#include "Cooler.h"
#include <cstdint>
#include <iomanip>

void Cooler::setPort(uint16_t port) { this->port = port; }

void Cooler::StartApplication() {
  socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
  socket->Bind(InetSocketAddress(Ipv4Address::GetAny(), port));
  socket->SetRecvCallback(MakeCallback(&Cooler::HandleRead, this));

  std::cout << "ACTUATOR: Ready to receive commands" << std::endl;
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
    std::string command(packet->GetSize(), '\0');

    packet->CopyData(reinterpret_cast<uint8_t *>(&command[0]),
                     static_cast<uint32_t>(command.size()));

    std::cout << std::fixed << std::setprecision(3) << "["
              << Simulator::Now().GetSeconds()
              << " s] ACTUATOR: Received command: " << command << std::endl;

    if (command == "COOLER ON") {
      std::cout << "ACTUATOR: Cooler state = ON" << std::endl;
    } else if (command == "COOLER OFF") {
      std::cout << "ACTUATOR: Cooler state = OFF" << std::endl;
    } else {
      std::cout << "ACTUATOR: Unknown command: " << command << std::endl;
    }
  }
}
