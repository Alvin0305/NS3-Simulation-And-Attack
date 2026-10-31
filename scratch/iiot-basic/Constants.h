#pragma once

namespace constants {
constexpr char sensorIP[] = "10.1.1.1";
constexpr char plcIP[] = "10.1.1.2";
constexpr char coolerIP[] = "10.1.1.3";
constexpr char attackerIP[] = "10.1.1.4";

constexpr char commandString[] = "command";
constexpr char coolerOffCommand[] = "COOLER OFF";
constexpr char coolerOnCommand[] = "COOLER ON";

constexpr unsigned int attackerPort = 49152;

namespace protocol {
constexpr unsigned int udp = 17;
};

}; // namespace Constants
