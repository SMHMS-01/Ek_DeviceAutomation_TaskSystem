#pragma once

#include "IDevice.h"

#include <string>

namespace device_automation::infrastructure
{

class MockDevice : public IDevice
{
public:
    MockDevice() = default;
    ~MockDevice() override = default;

    bool connect() override
    {
        connected_ = true;
        return true;
    }
    void disconnect() override
    {
        connected_ = false;
    }
    bool send(const std::string &payload) override
    {
        last_ = payload;
        return true;
    }
    std::string receive() override
    {
        return last_;
    }

private:
    bool connected_ = false;
    std::string last_;
};

} // namespace device_automation::infrastructure
