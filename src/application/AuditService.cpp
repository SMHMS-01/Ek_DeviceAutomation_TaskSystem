#include "AuditService.h"

#include "domain/Types.h"

#include <sstream>

namespace device_automation::application {

namespace {

std::string quote(const std::string& value)
{
    std::string out = "'";
    for (char ch : value) {
        if (ch == '\'') {
            out += "''";
        } else {
            out += ch;
        }
    }
    out += "'";
    return out;
}

std::string value_or_empty(const device_automation::infrastructure::DatabaseRow& row,
                           const std::string& key)
{
    auto it = row.find(key);
    return it == row.end() ? "" : it->second;
}

} // namespace

AuditService::AuditService(device_automation::infrastructure::IDatabase& db) : db_(db) {}

std::vector<AuditEntry> AuditService::events_for_task(const std::string& task_id)
{
    auto rows = db_.query("SELECT id, type, task_id, workflow_id, before_state, after_state, "
                          "reason, occurred_at FROM audit_events WHERE task_id = " +
                          quote(task_id) + " ORDER BY occurred_at ASC, id ASC");
    std::vector<AuditEntry> entries;
    entries.reserve(rows.size());
    for (const auto& row : rows) {
        entries.push_back(AuditEntry{value_or_empty(row, "id"),
                                     value_or_empty(row, "type"),
                                     value_or_empty(row, "task_id"),
                                     value_or_empty(row, "workflow_id"),
                                     value_or_empty(row, "before_state"),
                                     value_or_empty(row, "after_state"),
                                     value_or_empty(row, "reason"),
                                     value_or_empty(row, "occurred_at")});
    }
    return entries;
}

std::string AuditService::replay_task_state(const std::string& task_id)
{
    auto entries = events_for_task(task_id);
    if (entries.empty()) {
        return "";
    }
    return entries.back().after_state;
}

int AuditService::recover_running_tasks()
{
    auto rows = db_.query("SELECT id, workflow_id, state FROM tasks WHERE state = 'Running'");
    int recovered = 0;
    for (const auto& row : rows) {
        const auto task_id = value_or_empty(row, "id");
        const auto workflow_id = value_or_empty(row, "workflow_id");
        db_.execute("UPDATE tasks SET state = 'Paused', error_message = 'Recovered after restart' "
                    "WHERE id = " +
                    quote(task_id));
        record_system_recovery(task_id, workflow_id, "Running", "Paused");
        ++recovered;
    }
    return recovered;
}

void AuditService::record_system_recovery(const std::string& task_id,
                                          const std::string& workflow_id,
                                          const std::string& before_state,
                                          const std::string& after_state)
{
    std::ostringstream sql;
    sql << "INSERT INTO audit_events(id, type, task_id, workflow_id, actor, before_state, "
           "after_state, reason, occurred_at) VALUES("
        << quote(device_automation::domain::EventId::generate().to_string())
        << ", 'SystemRecovered', " << quote(task_id) << ", " << quote(workflow_id)
        << ", 'system', " << quote(before_state) << ", " << quote(after_state)
        << ", 'Recovered interrupted running task after startup', "
        << device_automation::domain::Timestamp::now().millis() << ")";
    db_.execute(sql.str());
}

} // namespace device_automation::application
