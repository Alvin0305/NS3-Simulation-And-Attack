#include "Logger.h"

#include "ns3/simulator.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

std::mutex Logger::logMutex;

const char* Logger::componentName(Component component) {
    switch (component) {
        case Component::PLC:
            return "PLC";
        case Component::Attacker:
            return "Attacker";
        case Component::Sensor:
            return "Sensor";
        case Component::Cooler:
            return "Cooler";
    }

    return "Unknown";
}

bool Logger::debugEnabled() {
    const char* value = std::getenv("IIOT_DEBUG");
    return value && *value && std::strcmp(value, "0") != 0;
}

Logger::Logger(Component component, bool useNamedPipe)
    : component(component),
      useNamedPipe(useNamedPipe) {
    if (!useNamedPipe) {
        return;
    }

    pipePath = "pipes/iiot_" + std::string(componentName(component)) + ".fifo";

    if (::mkfifo(pipePath.c_str(), 0666) == -1 && errno != EEXIST) {
        std::cerr << "Logger: cannot create FIFO " << pipePath << ": " << std::strerror(errno) << '\n';
        useNamedPipe = false;
        return;
    }

    struct stat info{};

    if (::stat(pipePath.c_str(), &info) == -1 || !S_ISFIFO(info.st_mode)) {
        std::cerr << "Logger: invalid FIFO " << pipePath << '\n';
        useNamedPipe = false;
    }
}

Logger::~Logger() {
    if (pipeFd >= 0) {
        ::close(pipeFd);
    }
}

Logger& Logger::get(Component component) {
    static Logger plc(Component::PLC);
    static Logger attacker(Component::Attacker);
    static Logger sensor(Component::Sensor);
    static Logger cooler(Component::Cooler);

    switch (component) {
        case Component::PLC:
            return plc;
        case Component::Attacker:
            return attacker;
        case Component::Sensor:
            return sensor;
        case Component::Cooler:
            return cooler;
    }

    return plc;
}

void Logger::log(const char* level, const char* file, int line, const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);

    // ANSI colors
    constexpr const char* RESET = "\033[0m";
    constexpr const char* BOLD = "\033[1m";
    constexpr const char* GRAY = "\033[90m";
    constexpr const char* CYAN = "\033[36m";
    constexpr const char* GREEN = "\033[32m";
    constexpr const char* YELLOW = "\033[33m";
    constexpr const char* RED = "\033[31m";

    const char* levelColor = GREEN;

    if (std::strcmp(level, "WARN") == 0) {
        levelColor = YELLOW;
    } else if (std::strcmp(level, "ERROR") == 0) {
        levelColor = RED;
    } else if (std::strcmp(level, "DEBUG") == 0) {
        levelColor = CYAN;
    }

    const char* filename = std::strrchr(file, '/');
    filename = filename ? filename + 1 : file;

    std::ostringstream formatted;
    formatted << GRAY << "[" << std::fixed << std::setprecision(3) << ns3::Simulator::Now().GetSeconds() << " s]" << RESET << " "
              << BOLD << "[" << componentName(component) << "]" << RESET << " " << CYAN << "[" << filename << ":" << line << "]"
              << RESET << " " << levelColor << "[" << level << "]" << RESET << " " << message << "\n";

    std::string output = formatted.str();

    if (useNamedPipe) {
        // Attempt to connect only when a message is ready.
        // O_NONBLOCK prevents waiting indefinitely for a reader.
        if (pipeFd < 0) {
            pipeFd = ::open(pipePath.c_str(), O_WRONLY | O_NONBLOCK);
        }

        if (pipeFd >= 0) {
            std::size_t offset = 0;

            while (offset < output.size()) {
                ssize_t written = ::write(pipeFd, output.data() + offset, output.size() - offset);

                if (written > 0) {
                    offset += static_cast<std::size_t>(written);
                } else if (written == -1 && errno == EINTR) {
                    continue;
                } else {
                    ::close(pipeFd);
                    pipeFd = -1;
                    break;
                }
            }

            // If the FIFO write failed, still show the message on stderr.
            if (offset == output.size()) {
                return;
            }
        }
    }

    std::cerr << output << std::flush;
}
