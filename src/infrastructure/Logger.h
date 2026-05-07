#pragma once

#include <string>
#include <iostream>

namespace device_automation::infrastructure {

class Logger
{
public:
    enum class Level { Debug, Info, Warn, Error };

    static void log(Level level, const std::string& msg) {
        switch (level) {
            case Level::Debug: std::cerr << "[DEBUG] "; break;
            case Level::Info: std::cerr << "[INFO] "; break;
            case Level::Warn: std::cerr << "[WARN] "; break;
            case Level::Error: std::cerr << "[ERROR] "; break;
        }
        std::cerr << msg << std::endl;
    }
};

} // namespace device_automation::infrastructure
