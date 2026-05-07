#pragma once

#include <chrono>

namespace device_automation::infrastructure {

class WatchDog
{
public:
    WatchDog() = default;
    ~WatchDog() = default;

    void kick() { last_ = std::chrono::steady_clock::now(); }
    bool is_stale(std::chrono::milliseconds expiry) const {
        return (std::chrono::steady_clock::now() - last_) > expiry;
    }

private:
    std::chrono::steady_clock::time_point last_{std::chrono::steady_clock::now()};
};

} // namespace device_automation::infrastructure
