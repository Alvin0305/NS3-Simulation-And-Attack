#include "Coap.h"

#include <cstddef>
#include <cstdint>
#include <vector>

std::vector<uint8_t> Coap::Encode(const Message& message) {
    std::vector<uint8_t> bytes;

    // CoAP header:
    // 2 bits version | 2 bits type | 4 bits token length
    const uint8_t firstByte = static_cast<uint8_t>((VERSION << 6) | (message.type << 4) | message.token.size());

    bytes.push_back(firstByte);
    bytes.push_back(message.code);

    // Message ID: 16-bit big-endian.
    bytes.push_back(static_cast<uint8_t>(message.messageId >> 8));

    bytes.push_back(static_cast<uint8_t>(message.messageId & 0xFF));

    // Token.
    bytes.insert(bytes.end(), message.token.begin(), message.token.end());

    // Encode one URI-Path option.
    //
    // Option header:
    // 4-bit delta | 4-bit length
    //
    // URI-Path option number = 11.
    // Our paths are short enough to fit in the 4-bit length.

    if (!message.path.empty()) {
        if (message.path.size() > 12) {
            throw std::runtime_error("URI path too long for this encoder");
        }

        const uint8_t optionHeader = static_cast<uint8_t>((URI_PATH_OPTION << 4) | message.path.size());

        bytes.push_back(optionHeader);

        for (unsigned char character : message.path) {
            bytes.push_back(character);
        }
    }

    // Payload marker: 0xFF.
    if (!message.payload.empty()) {
        bytes.push_back(0xFF);

        for (unsigned char character : message.payload) {
            bytes.push_back(character);
        }
    }

    return bytes;
}

bool Coap::Decode(const uint8_t* data, std::size_t size, Message& message) {
    if (size < 4) {
        return false;
    }

    const uint8_t firstByte = data[0];

    const auto version = static_cast<uint8_t>(firstByte >> 6);

    const auto tokenLength = static_cast<uint8_t>(firstByte & 0x0F);

    if (version != VERSION || tokenLength > 8) {
        return false;
    }

    if (size < static_cast<std::size_t>(4 + tokenLength)) {
        return false;
    }

    message.type = static_cast<uint8_t>((firstByte >> 4) & 0x03);

    message.code = data[1];

    message.messageId = static_cast<uint16_t>((static_cast<uint16_t>(data[2]) << 8) | data[3]);

    message.token.assign(data + 4, data + 4 + tokenLength);

    message.path.clear();
    message.payload.clear();

    std::size_t position = 4 + tokenLength;

    uint16_t currentOptionNumber = 0;

    while (position < size) {
        // 0xFF marks the beginning of the payload.
        if (data[position] == 0xFF) {
            ++position;

            if (position >= size) {
                return false;
            }

            message.payload.assign(reinterpret_cast<const char*>(data + position), size - position);

            return true;
        }

        const uint8_t optionHeader = data[position++];

        const uint8_t delta = static_cast<uint8_t>(optionHeader >> 4);

        const uint8_t length = static_cast<uint8_t>(optionHeader & 0x0F);

        // This educational decoder handles only the basic
        // delta/length encoding, not extended option values.
        if (delta >= 13 || length >= 13) {
            return false;
        }

        currentOptionNumber += delta;

        if (position + length > size) {
            return false;
        }

        if (currentOptionNumber == URI_PATH_OPTION) {
            if (!message.path.empty()) {
                message.path += "/";
            }

            message.path.append(reinterpret_cast<const char*>(data + position), length);
        }

        position += length;
    }

    return true;
}

// Create a two-byte token from a message sequence number.
std::vector<uint8_t> Coap::MakeToken(uint16_t value) {
    return {static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value & 0xFF)};
}

// Construct a request.
Coap::Message Coap::MakePost(uint16_t messageId, const std::string& path, const std::string& payload) {
    Message message;

    message.type = CON;
    message.code = POST;
    message.messageId = messageId;
    message.token = MakeToken(messageId);
    message.path = path;
    message.payload = payload;

    return message;
}

// Construct a response that acknowledges a request.
Coap::Message Coap::MakeResponse(const Message& request, uint8_t responseCode) {
    Message response;

    response.type = ACK;
    response.code = responseCode;

    // Piggybacked ACK uses the request's Message ID
    // and copies its token.
    response.messageId = request.messageId;
    response.token = request.token;

    return response;
}

// Send a CoAP message over a UDP socket.
int Coap::Send(Ptr<Socket> socket, const Address& destination, const Message& message) {
    const std::vector<uint8_t> bytes = Encode(message);

    Ptr<Packet> packet = Create<Packet>(bytes.data(), static_cast<uint32_t>(bytes.size()));

    return socket->SendTo(packet, 0, destination);
}
