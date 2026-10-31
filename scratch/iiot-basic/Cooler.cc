#include "Cooler.h"
#include "Coap.h"
#include <cstdint>
#include <iomanip>

void Cooler::StartApplication() {
  socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
  socket->Bind(InetSocketAddress(Ipv4Address::GetAny(), Coap::PORT));
  socket->SetRecvCallback(MakeCallback(&Cooler::HandleRead, this));

  std::cout << "ACTUATOR: CoAP server started on UDP port " << Coap::PORT
            << std::endl;
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

    if (!Coap::Decode(bytes.data(), bytes.size(), request)) {
      std::cout << "ACTUATOR: Invalid CoAP request" << std::endl;

      continue;
    }

    std::cout << std::fixed << std::setprecision(3) << "["
              << Simulator::Now().GetSeconds()
              << " s] ACTUATOR: Received CoAP request" << " path=/"
              << request.path << " MID=" << request.messageId << std::endl;

    uint8_t responseCode = Coap::NOT_FOUND;

    if (request.type == Coap::CON && request.code == Coap::POST &&
        request.path == "command") {
      if (request.payload == "COOLER ON") {
        std::cout << "ACTUATOR: Cooler state = ON" << std::endl;
        responseCode = Coap::CHANGED;
      } else if (request.payload == "COOLER OFF") {
        std::cout << "ACTUATOR: Cooler state = OFF" << std::endl;
        responseCode = Coap::CHANGED;
      } else {
        std::cout << "ACTUATOR: Unknown command: " << request.payload
                  << std::endl;
      }

      Coap::Message response = Coap::MakeResponse(request, responseCode);

      Coap::Send(socket, senderAddress, response);
    }
  }
}
