#pragma once

#include "Task.h"

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace device_automation::domain {

class TaskGraphException : public std::runtime_error
{
public:
    explicit TaskGraphException(const std::string& message) : std::runtime_error(message) {}
};

class TaskGraph
{
public:
    WorkflowId id = WorkflowId::generate();
    std::string name = "workflow";
    int version = 1;
    GraphState state = GraphState::Created;

    void add_task(Task task);
    void add_dependency(const TaskId& prerequisite, const TaskId& dependent);

    bool contains(const TaskId& id) const;
    Task& task(const TaskId& id);
    const Task& task(const TaskId& id) const;

    std::vector<TaskId> topological_sort() const;
    bool has_cycle() const;
    std::vector<TaskId> ready_tasks() const;
    bool all_completed() const;

    const std::unordered_map<TaskId, Task>& tasks() const { return tasks_; }

private:
    std::unordered_map<TaskId, Task> tasks_;
};

} // namespace device_automation::domain
