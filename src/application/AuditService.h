#pragma once

#include "infrastructure/IDatabase.h"

#include <string>
#include <vector>

namespace device_automation::application
{

struct AuditEntry
{
    std::string id;
    std::string type;
    std::string task_id;
    std::string workflow_id;
    std::string before_state;
    std::string after_state;
    std::string reason;
    std::string occurred_at;
};

class AuditService
{
public:
    explicit AuditService(device_automation::infrastructure::IDatabase &db);

    std::vector<AuditEntry> events_for_task(const std::string &task_id);
    std::vector<AuditEntry> events_for_workflow(const std::string &workflow_id);
    std::string replay_task_state(const std::string &task_id);
    int recover_running_tasks();

private:
    void record_system_recovery(const std::string &task_id, const std::string &workflow_id,
                                const std::string &before_state, const std::string &after_state);

    device_automation::infrastructure::IDatabase &db_;
};

} // namespace device_automation::application
