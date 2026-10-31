#include "Auth.h"

#include <array>
#include <cstddef>
#include <iomanip>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <sstream>

namespace {
constexpr char SHARED_SECRET[] = "ns3-iiot-demo-key-abcdefghijklmnopqrstuvwxy";

constexpr std::size_t SHA256_LENGTH = 32;

std::string ToHex(const unsigned char *data, std::size_t length) {
  std::ostringstream output;
  output << std::hex << std::setfill('0');

  for (std::size_t i = 0; i < length; ++i) {
    output << std::setw(2) << static_cast<unsigned int>(data[i]);
  }

  return output.str();
}

bool FromHex(const std::string &hex,
             std::array<unsigned char, SHA256_LENGTH> &output) {
  if (hex.size() != SHA256_LENGTH * 2) {
    return false;
  }

  auto hexValue = [](char c) -> int {
    if (c >= '0' && c <= '9')
      return c - '0';
    if (c >= 'a' && c <= 'f')
      return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
      return c - 'A' + 10;
    return -1;
  };

  for (std::size_t i = 0; i < SHA256_LENGTH; ++i) {
    const int high = hexValue(hex[2 * i]);
    const int low = hexValue(hex[2 * i + 1]);

    if (high < 0 || low < 0) {
      return false;
    }

    output[i] = static_cast<unsigned char>((high << 4) | low);
  }

  return true;
}
}; // namespace

std::string Auth::ComputeTag(const std::string &message) {
  std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
  unsigned int digestLength = 0;

  const auto *result = HMAC(
      EVP_sha256(), SHARED_SECRET, static_cast<int>(sizeof(SHARED_SECRET) - 1),
      reinterpret_cast<const unsigned char *>(message.data()), message.size(),
      digest.data(), &digestLength);
  
  if (result == nullptr || digestLength != SHA256_LENGTH) {
    return {};
  }

  return ToHex(digest.data(), digestLength);
}

bool Auth::VerifyTag(const std::string &message,
                     const std::string &receivedTag) {
  std::array<unsigned char, SHA256_LENGTH> received{};
  if (not FromHex(receivedTag, received)) {
    return false;
  }

  const std::string expectedTag = ComputeTag(message);

  std::array<unsigned char, SHA256_LENGTH> expected{};
  if (expectedTag.empty() || not FromHex(expectedTag, expected)) {
    return false;
  }

  return CRYPTO_memcmp(expected.data(), received.data(), SHA256_LENGTH) == 0;
}
