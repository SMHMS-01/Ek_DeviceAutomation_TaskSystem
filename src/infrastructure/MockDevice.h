#pragma once

#include "IDevice.h"

#include <mutex>
#include <string>
#include <vector>

namespace device_automation::infrastructure
{

class MockDevice : public IDevice
{
public:
    MockDevice() = default;
    ~MockDevice() override = default;

    bool connect() override
    {
        std::lock_guard<std::mutex> lock(mu_);
        connected_ = true;
        return true;
    }
    void disconnect() override
    {
        std::lock_guard<std::mutex> lock(mu_);
        connected_ = false;
    }
    bool send(const std::string &payload) override
    {
        std::lock_guard<std::mutex> lock(mu_);
        if (!connected_)
        {
            return false;
        }
        last_ = payload;
        sent_payloads_.push_back(payload);
        return true;
    }
    std::string receive() override
    {
        std::lock_guard<std::mutex> lock(mu_);
        return last_;
    }

    bool is_connected() const
    {
        std::lock_guard<std::mutex> lock(mu_);
        return connected_;
    }

    std::vector<std::string> sent_payloads() const
    {
        std::lock_guard<std::mutex> lock(mu_);
        return sent_payloads_;
    }

private:
    mutable std::mutex mu_;
    bool connected_ = false;
    std::string last_;
    std::vector<std::string> sent_payloads_;
};

} // namespace device_automation::infrastructure
