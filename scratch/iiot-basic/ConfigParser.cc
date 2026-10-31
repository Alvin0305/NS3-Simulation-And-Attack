#include "ConfigParser.h"
#include "Config.h"
#include "Logger.h"
#include "nlohmann/json.hpp"
#include <exception>
#include <fstream>
#include <stdexcept>
#include <string>

using json = nlohmann::json;

IIoTConfig ConfigParser::parse(const std::string configFileName) const {
  std::ifstream f(configFileName);

  if (not f.is_open()) {
    throw std::runtime_error(std::string("There is no file: ") +
                             configFileName);
  }

  try {
    IIoTConfig config;

    json j = json::parse(f);
    config.doDirectCommandInjection =
        j.at("do-direct-command-injection").get<bool>();
    config.sendSpoofedPacket = j.at("send-spoofed-packet").get<bool>();

    return config;
  } catch (const std::exception &e) {
    LOG_ERROR("Config parsing failed: {}", e.what());
    return {false, false};
  }
}
