#pragma once

#include <string>
#include <iostream>
#include <mutex>

#ifdef USE_SPDLOG
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#endif

namespace device_automation::infrastructure {

class Logger
{
public:
    enum class Level { Debug, Info, Warn, Error };

    static void log(Level level, const std::string& msg) {
#ifdef USE_SPDLOG
        ensure_logger();
        auto logger = spdlog::get("device_logger");
        if (!logger) {
            // Fallback to spdlog default logger
            spdlog::info("{}", msg);
            return;
        }
        switch (level) {
            case Level::Debug: logger->debug(msg); break;
            case Level::Info:  logger->info(msg);  break;
            case Level::Warn:  logger->warn(msg);  break;
            case Level::Error: logger->error(msg); break;
        }
#else
        switch (level) {
            case Level::Debug: std::cerr << "[DEBUG] "; break;
            case Level::Info:  std::cerr << "[INFO]  "; break;
            case Level::Warn:  std::cerr << "[WARN]  "; break;
            case Level::Error: std::cerr << "[ERROR] "; break;
        }
        std::cerr << msg << std::endl;
#endif
    }

private:
#ifdef USE_SPDLOG
    static void ensure_logger() {
        static std::once_flag once;
        std::call_once(once, [](){
            if (!spdlog::get("device_logger")) {
                auto lg = spdlog::stdout_color_mt("device_logger");
                lg->set_level(spdlog::level::info);
                spdlog::set_default_logger(lg);
            }
        });
    }
#endif
};

} // namespace device_automation::infrastructure
