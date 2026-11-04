#pragma once

#include "ns3/core-module.h"
#include "ns3/internet-module.h"

#include <cstdint>
#include <string>
#include <vector>

using namespace ns3;

namespace Coap {
    constexpr uint8_t VERSION = 1;

    constexpr uint8_t CON = 0;
    constexpr uint8_t ACK = 2;

    constexpr uint8_t POST = 2;

    // 2.04 Changed
    constexpr uint8_t CHANGED = 68;
    // 4.03 Forbidden
    constexpr uint8_t FORBIDDEN = 131;
    // 4.04 Not Found
    constexpr uint8_t NOT_FOUND = 132;

    constexpr uint16_t PORT = 5683;

    constexpr uint16_t URI_PATH_OPTION = 11;

    struct Message {
        uint8_t type = CON;
        uint8_t code = 0;

        uint16_t messageId = 0;

        std::vector<uint8_t> token;
        std::string path;
        std::string payload;
    };

    std::vector<uint8_t> Encode(const Message& message);
    bool Decode(const uint8_t* data, std::size_t size, Message& message);

    std::vector<uint8_t> MakeToken(uint16_t value);
    Message MakePost(uint16_t messageId, const std::string& path, const std::string& payload);
    Message MakeResponse(const Message& request, uint8_t responseCode);

    int Send(Ptr<Socket> socket, const Address& destination, const Message& message);
}; // namespace Coap
