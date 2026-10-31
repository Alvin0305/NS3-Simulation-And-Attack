#pragma once

#include "Config.h"
#include <string>

class ConfigParser {
public:
  ConfigParser() = default;

  IIoTConfig parse(const std::string configFileName) const;
};
