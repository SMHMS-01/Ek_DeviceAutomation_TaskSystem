#include "TaskGraph.h"

#include <algorithm>
#include <queue>
#include <unordered_map>

namespace device_automation::domain {

void TaskGraph::add_task(Task task)
{
    if (task.id.to_string().empty()) {
        task.id = TaskId::generate();
    }
    task.workflow_id = id;
    task.version = version;
    tasks_.emplace(task.id, std::move(task));
}

void TaskGraph::add_dependency(const TaskId& prerequisite, const TaskId& dependent)
{
    if (!contains(prerequisite) || !contains(dependent)) {
        throw TaskGraphException("dependency references an unknown task");
    }
    auto& before = task(prerequisite);
    auto& after = task(dependent);
    before.dependents.push_back(dependent);
    after.dependencies.push_back(prerequisite);
    if (has_cycle()) {
        before.dependents.pop_back();
        after.dependencies.pop_back();
        throw TaskGraphException("dependency introduces a cycle");
    }
}

bool TaskGraph::contains(const TaskId& id) const
{
    return tasks_.find(id) != tasks_.end();
}

Task& TaskGraph::task(const TaskId& id)
{
    auto it = tasks_.find(id);
    if (it == tasks_.end()) {
        throw TaskGraphException("task not found: " + id.to_string());
    }
    return it->second;
}

const Task& TaskGraph::task(const TaskId& id) const
{
    auto it = tasks_.find(id);
    if (it == tasks_.end()) {
        throw TaskGraphException("task not found: " + id.to_string());
    }
    return it->second;
}

std::vector<TaskId> TaskGraph::topological_sort() const
{
    std::unordered_map<TaskId, int> indegree;
    for (const auto& [id, task] : tasks_) {
        indegree[id] = static_cast<int>(task.dependencies.size());
    }

    std::queue<TaskId> ready;
    for (const auto& [id, degree] : indegree) {
        if (degree == 0) {
            ready.push(id);
        }
    }

    std::vector<TaskId> sorted;
    while (!ready.empty()) {
        auto id = ready.front();
        ready.pop();
        sorted.push_back(id);

        for (const auto& dependent : task(id).dependents) {
            auto next = indegree.find(dependent);
            if (next != indegree.end() && --next->second == 0) {
                ready.push(dependent);
            }
        }
    }

    if (sorted.size() != tasks_.size()) {
        throw TaskGraphException("task graph contains a cycle");
    }
    return sorted;
}

bool TaskGraph::has_cycle() const
{
    try {
        (void)topological_sort();
        return false;
    } catch (const TaskGraphException&) {
        return true;
    }
}

std::vector<TaskId> TaskGraph::ready_tasks() const
{
    std::vector<TaskId> ready;
    for (const auto& [id, candidate] : tasks_) {
        if (candidate.state != TaskState::Pending) {
            continue;
        }
        const bool dependencies_done =
            std::all_of(candidate.dependencies.begin(), candidate.dependencies.end(),
                        [this](const TaskId& dependency) {
                            return task(dependency).state == TaskState::Completed;
                        });
        if (dependencies_done) {
            ready.push_back(id);
        }
    }
    std::sort(ready.begin(), ready.end(), [this](const TaskId& lhs, const TaskId& rhs) {
        const auto& left = task(lhs);
        const auto& right = task(rhs);
        if (left.priority == right.priority) {
            return left.created_at < right.created_at;
        }
        return left.priority < right.priority;
    });
    return ready;
}

bool TaskGraph::all_completed() const
{
    return std::all_of(tasks_.begin(), tasks_.end(), [](const auto& item) {
        return item.second.state == TaskState::Completed;
    });
}

} // namespace device_automation::domain
