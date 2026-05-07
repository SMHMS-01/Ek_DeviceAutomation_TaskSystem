#include "EventBus.h"
#include "Logger.h"

#include <mutex>
#include <algorithm>

namespace device_automation::infrastructure {

int EventBus::subscribe(const std::string& key, EventCallback cb)
{
    std::lock_guard<std::mutex> lk(mu_);
    int id = next_id_++;
    subs_[key].push_back({id, std::move(cb)});
    return id;
}

void EventBus::unsubscribe(const std::string& key, int subscription_id)
{
    std::lock_guard<std::mutex> lk(mu_);
    auto it = subs_.find(key);
    if (it == subs_.end()) return;
    auto &vec = it->second;
    vec.erase(std::remove_if(vec.begin(), vec.end(), [subscription_id](const EventBus::Subscriber& s){ return s.id == subscription_id; }), vec.end());
}

void EventBus::publish(const std::string& key, const std::string& payload)
{
    std::vector<EventCallback> cbs;
    {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = subs_.find(key);
        if (it == subs_.end()) return;
        cbs.reserve(it->second.size());
        for (const auto &s : it->second) cbs.push_back(s.cb);
    }

    for (auto &cb : cbs) {
        try {
            cb(payload);
        } catch (const std::exception &e) {
            Logger::log(Logger::Level::Error, std::string("EventBus callback error: ") + e.what());
        } catch (...) {
            Logger::log(Logger::Level::Error, "EventBus callback unknown error");
        }
    }
}

// (implementation complete)

} // namespace device_automation::infrastructure
