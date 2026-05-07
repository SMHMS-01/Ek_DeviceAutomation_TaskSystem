#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace device_automation::infrastructure {

using EventCallback = std::function<void(const std::string& payload)>;

class EventBus
{
public:
    EventBus() = default;
    ~EventBus() = default;

    // Subscriber entry with id
    struct Subscriber { int id; EventCallback cb; };

    // Subscribe to an event key, returns subscription id
    int subscribe(const std::string& key, EventCallback cb);

    // Unsubscribe
    void unsubscribe(const std::string& key, int subscription_id);

    // Publish an event to subscribers
    void publish(const std::string& key, const std::string& payload);

private:
    std::unordered_map<std::string, std::vector<Subscriber>> subs_;
    std::mutex mu_;
    int next_id_ = 1;
};

} // namespace device_automation::infrastructure
