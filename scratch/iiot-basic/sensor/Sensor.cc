#include "Sensor.h"

#include "logger/Logger.h"
#include "protocol/Coap.h"

#include <cstdint>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include <vector>

void Sensor::SetDestination(Ipv4Address address) {
    this->destinationAddress = address;
}

void Sensor::StartApplication() {
    socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
    socket->Bind();

    socket->SetRecvCallback(MakeCallback(&Sensor::HandleResponse, this));
    SENSOR_LOG_INFO("CoAP client started");

    sendEvent = Simulator::ScheduleNow(&Sensor::SendReading, this);
}

void Sensor::StopApplication() {
    Simulator::Cancel(sendEvent);

    if (socket) {
        socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
        socket->Close();
        socket = nullptr;
    }
}

void Sensor::SendReading() {
    static const double readings[] = {65.0, 85.0, 85.0, 55.0};

    const uint32_t index = (sequence - 1) % 4;
    const double temperature = readings[index];

    std::ostringstream output;
    output << std::fixed << std::setprecision(1) << temperature;

    const std::string payload = output.str();
    Coap::Message request = Coap::MakePost(nextMessageId++, "temperature", payload);

    const Address destination = InetSocketAddress(destinationAddress, Coap::PORT);

    Coap::Send(socket, destination, request);

    SENSOR_LOG_INFO("CoAP POST /temperature payload={}, MID={}", payload, request.messageId);

    sequence++;

    sendEvent = Simulator::Schedule(Seconds(1.0), &Sensor::SendReading, this);
}

void Sensor::HandleResponse(Ptr<Socket> socket) {
    Address senderAddress;
    Ptr<Packet> packet;

    while ((packet = socket->RecvFrom(senderAddress))) {
        std::vector<uint8_t> bytes(packet->GetSize());

        packet->CopyData(bytes.data(), static_cast<uint32_t>(bytes.size()));

        Coap::Message response;

        if (not Coap::Decode(bytes.data(), bytes.size(), response)) {
            SENSOR_LOG_ERROR("Invalid CoAP response");
            std::cout << "SENSOR: Invalid CoAP response" << std::endl;
            continue;
        }

        SENSOR_LOG_INFO("Received CoAP response code={}, MID={}", static_cast<int>(response.code), response.messageId);
    }
}
