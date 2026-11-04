#ifndef LOGGER_H
#define LOGGER_H

#include "utils/Formatter.h"

#include <format>
#include <mutex>
#include <string>
#include <utility>

class Logger {
  public:
    enum class Component {
        PLC,
        Attacker,
        Sensor,
        Cooler
    };

    static Logger& get(Component component);

    static Logger& PLC() {
        return get(Component::PLC);
    }

    static Logger& Attacker() {
        return get(Component::Attacker);
    }

    static Logger& Sensor() {
        return get(Component::Sensor);
    }

    static Logger& Cooler() {
        return get(Component::Cooler);
    }

    template <typename... Args>
    void info(const char* file, int line, std::format_string<Args...> fmt, Args&&... args) {
        log("INFO", file, line, std::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void warn(const char* file, int line, std::format_string<Args...> fmt, Args&&... args) {
        log("WARN", file, line, std::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void error(const char* file, int line, std::format_string<Args...> fmt, Args&&... args) {
        log("ERROR", file, line, std::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void debug(const char* file, int line, std::format_string<Args...> fmt, Args&&... args) {
        if (debugEnabled()) {
            log("DEBUG", file, line, std::format(fmt, std::forward<Args>(args)...));
        }
    }

  private:
    explicit Logger(Component component, bool useNamedPipe = true);
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void log(const char* level, const char* file, int line, const std::string& message);

    static const char* componentName(Component component);
    static bool debugEnabled();

    Component component;
    bool useNamedPipe;
    int pipeFd = -1;
    std::string pipePath;

    static std::mutex logMutex;
};

// Generic logging defaults to PLC.
#define LOG_INFO(...) Logger::PLC().info(__FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...) Logger::PLC().warn(__FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) Logger::PLC().error(__FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) Logger::PLC().debug(__FILE__, __LINE__, __VA_ARGS__)

#define PLC_LOG_INFO(...) Logger::PLC().info(__FILE__, __LINE__, __VA_ARGS__)
#define PLC_LOG_WARN(...) Logger::PLC().warn(__FILE__, __LINE__, __VA_ARGS__)
#define PLC_LOG_ERROR(...) Logger::PLC().error(__FILE__, __LINE__, __VA_ARGS__)
#define PLC_LOG_DEBUG(...) Logger::PLC().debug(__FILE__, __LINE__, __VA_ARGS__)

#define SENSOR_LOG_INFO(...) Logger::Sensor().info(__FILE__, __LINE__, __VA_ARGS__)
#define SENSOR_LOG_WARN(...) Logger::Sensor().warn(__FILE__, __LINE__, __VA_ARGS__)
#define SENSOR_LOG_ERROR(...) Logger::Sensor().error(__FILE__, __LINE__, __VA_ARGS__)
#define SENSOR_LOG_DEBUG(...) Logger::Sensor().debug(__FILE__, __LINE__, __VA_ARGS__)

#define ATTACKER_LOG_INFO(...) Logger::Attacker().info(__FILE__, __LINE__, __VA_ARGS__)
#define ATTACKER_LOG_WARN(...) Logger::Attacker().warn(__FILE__, __LINE__, __VA_ARGS__)
#define ATTACKER_LOG_ERROR(...) Logger::Attacker().error(__FILE__, __LINE__, __VA_ARGS__)
#define ATTACKER_LOG_DEBUG(...) Logger::Attacker().debug(__FILE__, __LINE__, __VA_ARGS__)

#define COOLER_LOG_INFO(...) Logger::Cooler().info(__FILE__, __LINE__, __VA_ARGS__)
#define COOLER_LOG_WARN(...) Logger::Cooler().warn(__FILE__, __LINE__, __VA_ARGS__)
#define COOLER_LOG_ERROR(...) Logger::Cooler().error(__FILE__, __LINE__, __VA_ARGS__)
#define COOLER_LOG_DEBUG(...) Logger::Cooler().debug(__FILE__, __LINE__, __VA_ARGS__)

#endif
