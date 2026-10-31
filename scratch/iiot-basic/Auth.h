#pragma once

#include <string>

namespace Auth {

std::string ComputeTag(const std::string &message);

bool VerifyTag(const std::string &message, const std::string &receivedTag);

} // namespace Auth
