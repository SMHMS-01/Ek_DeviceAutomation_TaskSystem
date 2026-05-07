#pragma once

#include <string>

namespace device_automation::infrastructure {

class PluginLoader
{
public:
    PluginLoader() = default;
    ~PluginLoader() = default;

    // Attempt to load a plugin (path to .so/.dll)
    bool load(const std::string& path);
};

} // namespace device_automation::infrastructure
