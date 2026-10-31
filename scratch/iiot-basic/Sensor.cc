#include "Sensor.h"
#include <cstdint>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>

void Sensor::SetDestination(Ipv4Address address, uint16_t port) {
  this->destinationAddress = address;
  this->destinationPort = port;
}

void Sensor::SetInterval(Time interval) { this->interval = interval; }

void Sensor::StartApplication() {
  socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
  socket->Bind();

  // start sending readings as soon as the application starts
  sequence = 1;

  sendEvent = Simulator::ScheduleNow(&Sensor::SendReading, this);
}

void Sensor::StopApplication() {
  Simulator::Cancel(sendEvent);

  if (socket) {
    socket->Close();
    socket = nullptr;
  }
}

void Sensor::SendReading() {
  static const double readings[] = {65.0, 85.0, 85.0, 55.0};

  const uint32_t index = (sequence - 1) % 4;
  const double temperature = readings[index];

  std::ostringstream output;
  output << "TEMP seq=" << sequence << " temperature=" << std::fixed
         << std::setprecision(1) << temperature;

  const std::string message = output.str();

  Ptr<Packet> packet =
      Create<Packet>(reinterpret_cast<const uint8_t *>(message.data()),
                     static_cast<uint32_t>(message.size()));

  // 0 is flag
  socket->SendTo(packet, 0,
                 InetSocketAddress(destinationAddress, destinationPort));

  std::cout << std::fixed << std::setprecision(3) << "["
            << Simulator::Now().GetSeconds() << " s] SENSOR: Sent " << message
            << std::endl;

  sequence++;

  sendEvent = Simulator::Schedule(interval, &Sensor::SendReading, this);
}
