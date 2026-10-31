#pragma once

#include "IIoTConfig.h"
#include <string>

class ConfigParser {
public:
  ConfigParser() = default;

  IIoTConfig parse(const std::string configFileName) const;
};
