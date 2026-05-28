#pragma once

#include <string>

namespace device_automation::infrastructure
{

class IDevice
{
public:
    virtual ~IDevice() = default;

    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool send(const std::string &payload) = 0;
    virtual std::string receive() = 0;
    virtual std::string status()
    {
        return receive();
    }
};

} // namespace device_automation::infrastructure
