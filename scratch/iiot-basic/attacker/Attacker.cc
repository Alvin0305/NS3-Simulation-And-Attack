#include "Attacker.h"

#include "Constants.h"
#include "logger/Logger.h"
#include "protocol/Coap.h"
#include "utils/IIoTConfig.h"

#include "ns3/boolean.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-raw-socket-factory.h"
#include "ns3/simulator.h"
#include "ns3/udp-header.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/uinteger.h"

#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <vector>

namespace ns3 {
    NS_OBJECT_ENSURE_REGISTERED(Attacker);

    TypeId Attacker::GetTypeId() {
        static TypeId id = TypeId("ns3::Attacker").SetParent<Application>().SetGroupName("Application");

        return id;
    }

    Attacker::Attacker(const IIoTConfig& config)
        : config(config),
          targetAddress(Ipv4Address::GetAny()),
          messageId(500) {};

    Attacker::~Attacker() = default;

    void Attacker::SetTarget(Ipv4Address address) {
        targetAddress = address;
    }

    void Attacker::StartApplication() {
        socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
        socket->Bind();
        socket->SetRecvCallback(MakeCallback(&Attacker::HandleRead, this));

        for (uint32_t i = 0; i < GetNode()->GetNDevices(); i++) {
            GetNode()->GetDevice(i)->SetPromiscReceiveCallback(MakeCallback(&Attacker::PromiscSniffer, this));
        }

        ATTACKER_LOG_INFO("Promiscous packet capture enabled");

        Simulator::Schedule(Seconds(2.5), &Attacker::SendConfiguredAttacks, this);

        if (config.sendReplayAttack) {
            Simulator::Schedule(Seconds(4.5), &Attacker::SendReplayAttack, this);
        }
    }

    void Attacker::StopApplication() {
        if (socket) {
            socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
            socket->Close();
            socket = nullptr;
        }

        if (rawSocket) {
            rawSocket->Close();
            rawSocket = nullptr;
        }
    }

    void Attacker::HandleRead(Ptr<Socket> socket) {
        Address senderAddress;
        Ptr<Packet> packet;

        while ((packet = socket->RecvFrom(senderAddress))) {
            std::vector<uint8_t> bytes(packet->GetSize());
            packet->CopyData(bytes.data(), static_cast<uint32_t>(bytes.size()));

            Coap::Message response;
            if (not Coap::Decode(bytes.data(), bytes.size(), response)) {
                ATTACKER_LOG_ERROR("Received an invalid CoAP response");
                std::cout << "ATTACKER: Received an invalid CoAP response" << std::endl;
                continue;
            }

            ATTACKER_LOG_INFO("Received response code={}, MID={}", static_cast<unsigned>(response.code), response.messageId);

            switch (response.code) {
                case Coap::CHANGED:
                    ATTACKER_LOG_INFO("Command accepted by the cooler. MID={}", response.messageId);
                    break;

                case Coap::FORBIDDEN:
                    ATTACKER_LOG_WARN("Command REJECTED by the cooler: unauthorized sender. MID={}", response.messageId);
                    break;

                case Coap::NOT_FOUND:
                    ATTACKER_LOG_WARN("Command failed: resource not found. MID={}", response.messageId);
                    break;

                default:
                    ATTACKER_LOG_WARN("Received an unexpected CoAP response code={} for MID={}",
                                      static_cast<unsigned>(response.code),
                                      response.messageId);
                    break;
            }
        }
    }

    void Attacker::SendSpoofedCommand() {
        Coap::Message command = Coap::MakePost(messageId, constants::commandString, constants::coolerOffCommand);

        const std::vector<uint8_t> bytes = Coap::Encode(command);
        SendSpoofedCoapBytes(bytes, messageId, "unauthenticated spoofed");
    }

    bool Attacker::PromiscSniffer(Ptr<NetDevice> device,
                                  Ptr<const Packet> packet,
                                  uint16_t protocol,
                                  const Address& from,
                                  const Address& to,
                                  NetDevice::PacketType packetType) {
        if (protocol != 0x0800) {
            return false;
        }

        Ptr<Packet> copy = packet->Copy();

        Ipv4Header ip;
        if (copy->RemoveHeader(ip) == 0) {
            return false;
        }

        if (ip.GetSource() != Ipv4Address(constants::plcIP) || ip.GetDestination() != Ipv4Address(constants::coolerIP) ||
            ip.GetProtocol() != constants::protocol::udp) {
            return false;
        }

        UdpHeader udp;
        if (copy->RemoveHeader(udp) == 0) {
            return false;
        }

        if (udp.GetDestinationPort() != Coap::PORT) {
            return false;
        }

        std::vector<uint8_t> bytes(copy->GetSize());
        if (bytes.empty()) {
            return false;
        }

        copy->CopyData(bytes.data(), static_cast<uint32_t>(bytes.size()));

        Coap::Message message;
        if (!Coap::Decode(bytes.data(), bytes.size(), message)) {
            return false;
        }

        if (message.type != Coap::CON || message.code != Coap::POST || message.path != constants::commandString) {
            return false;
        }

        const size_t first = message.payload.find('|');
        const size_t second = first == std::string::npos ? std::string::npos : message.payload.find('|', first + 1);

        if (first == std::string::npos || second == std::string::npos || message.payload.size() - second - 1 != 64) {
            return false;
        }

        capturedCoapBytes = std::move(bytes);
        capturedMessageId = message.messageId;
        hasCapturedCommand = true;

        ATTACKER_LOG_INFO("Captured PLC command for replay: source={}, MID={}, payload={}",
                          constants::plcIP,
                          capturedMessageId,
                          message.payload);

        return false;
    }

    void Attacker::SendSpoofedCoapBytes(const std::vector<uint8_t>& coapBytes, uint16_t mid, const std::string& attackName) {
        const Ipv4Address forgedSource(constants::plcIP);
        const Ipv4Address destination(constants::coolerIP);

        Ptr<Packet> packet = Create<Packet>(coapBytes.data(), static_cast<uint32_t>(coapBytes.size()));

        UdpHeader udp;
        udp.SetSourcePort(constants::attackerPort);
        udp.SetDestinationPort(Coap::PORT);
        udp.EnableChecksums();
        udp.InitializeChecksum(forgedSource, destination, constants::protocol::udp);

        packet->AddHeader(udp);

        Ipv4Header ip;
        ip.SetSource(forgedSource);
        ip.SetDestination(destination);
        ip.SetProtocol(constants::protocol::udp);
        ip.SetTtl(64);
        ip.SetIdentification(mid);
        ip.SetPayloadSize(packet->GetSize());

        packet->AddHeader(ip);

        if (!rawSocket) {
            rawSocket = Socket::CreateSocket(GetNode(), Ipv4RawSocketFactory::GetTypeId());

            rawSocket->SetAttribute("Protocol", UintegerValue(constants::protocol::udp));

            rawSocket->SetAttribute("IpHeaderInclude", BooleanValue(true));
        }

        const Address target = InetSocketAddress(destination, 0);
        const int result = rawSocket->SendTo(packet, 0, target);

        if (result < 0) {
            ATTACKER_LOG_ERROR("Failed to send {} attack", attackName);
        } else {
            ATTACKER_LOG_INFO("Sent {} packet: forged_source={}, destination={}, MID={}",
                              attackName,
                              constants::plcIP,
                              constants::coolerIP,
                              mid);
        }
    }

    void Attacker::SendInvalidHmacAttack() {
        const std::string invalidPayload = "COOLER OFF|99|" + std::string(64, '0');

        Coap::Message request = Coap::MakePost(messageId, constants::commandString, invalidPayload);

        const std::vector<uint8_t> bytes = Coap::Encode(request);

        ATTACKER_LOG_INFO("Attempting invalid-HMAC attack: payload={}, MID={}", invalidPayload, messageId);

        SendSpoofedCoapBytes(bytes, messageId, "invalid-HMAC");

        ++messageId;
    }

    void Attacker::SendReplayAttack() {
        if (!hasCapturedCommand || capturedCoapBytes.empty()) {
            ATTACKER_LOG_WARN("Replay attack skipped: no authenticated PLC command captured");
            return;
        }

        ATTACKER_LOG_INFO("Attempting replay attack using captured PLC command: MID={}", capturedMessageId);

        SendSpoofedCoapBytes(capturedCoapBytes, capturedMessageId, "replay");

        ATTACKER_LOG_INFO("Replayed original CoAP request with its original MID and HMAC");
    }

    void Attacker::SendConfiguredAttacks() {
        if (config.doDirectCommandInjection) {
            ATTACKER_LOG_INFO("Sending unauthenticated command injection");

            Coap::Message request = Coap::MakePost(messageId, constants::commandString, constants::coolerOffCommand);

            const Address destination = InetSocketAddress(targetAddress, Coap::PORT);

            const int result = Coap::Send(socket, destination, request);

            if (result < 0) {
                ATTACKER_LOG_ERROR("Failed to send unauthenticated command");
            } else {
                ATTACKER_LOG_INFO("Sent unauthenticated POST /command, MID={}", messageId);
            }

            ++messageId;
        }

        if (config.sendSpoofedPacket) {
            ATTACKER_LOG_INFO("Sending spoofed command without HMAC");

            SendSpoofedCommand();
            ++messageId;
        }

        if (config.sendInvalidHMAC) {
            SendInvalidHmacAttack();
        }
    }

}; // namespace ns3
