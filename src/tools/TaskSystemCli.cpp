#include "application/WorkflowManager.h"
#include "domain/TaskGraph.h"
#include "infrastructure/EventBus.h"
#include "infrastructure/SqliteDatabase.h"
#include "scheduler/InlineExecutor.h"
#include "scheduler/SimpleScheduler.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{

std::vector<std::string> split(const std::string &value, char delimiter)
{
    std::vector<std::string> parts;
    std::stringstream stream(value);
    std::string part;
    while (std::getline(stream, part, delimiter))
    {
        if (!part.empty())
        {
            parts.push_back(part);
        }
    }
    return parts;
}

device_automation::domain::Priority parse_priority(const std::string &value)
{
    if (value == "Critical")
    {
        return device_automation::domain::Priority::Critical;
    }
    if (value == "High")
    {
        return device_automation::domain::Priority::High;
    }
    if (value == "Low")
    {
        return device_automation::domain::Priority::Low;
    }
    if (value == "Background")
    {
        return device_automation::domain::Priority::Background;
    }
    return device_automation::domain::Priority::Normal;
}

struct CsvTask
{
    std::string key;
    std::string name;
    device_automation::domain::Priority priority = device_automation::domain::Priority::Normal;
    std::vector<std::string> dependencies;
};

std::vector<CsvTask> load_tasks(const std::string &path)
{
    std::ifstream input(path);
    if (!input)
    {
        throw std::runtime_error("cannot open workflow file: " + path);
    }

    std::vector<CsvTask> tasks;
    std::string line;
    while (std::getline(input, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        auto columns = split(line, ',');
        if (columns.size() < 3)
        {
            throw std::runtime_error("invalid workflow row: " + line);
        }

        CsvTask task;
        task.key = columns[0];
        task.name = columns[1];
        task.priority = parse_priority(columns[2]);
        if (columns.size() >= 4)
        {
            task.dependencies = split(columns[3], ';');
        }
        tasks.push_back(std::move(task));
    }
    return tasks;
}

device_automation::domain::TaskGraph build_graph(const std::vector<CsvTask> &csv_tasks)
{
    device_automation::domain::TaskGraph graph;
    graph.name = "cli-sample-workflow";
    std::unordered_map<std::string, device_automation::domain::TaskId> ids;

    for (const auto &csv_task : csv_tasks)
    {
        auto task = device_automation::domain::make_atomic_task(csv_task.name, csv_task.priority);
        ids.emplace(csv_task.key, task.id);
        graph.add_task(std::move(task));
    }

    for (const auto &csv_task : csv_tasks)
    {
        auto dependent = ids.at(csv_task.key);
        for (const auto &dependency_key : csv_task.dependencies)
        {
            graph.add_dependency(ids.at(dependency_key), dependent);
        }
    }
    return graph;
}

} // namespace

int main(int argc, char **argv)
{
    const std::string workflow_path =
        argc > 1 ? argv[1] : "tests/fixtures/sample_workflow_linear.csv";
    const std::string db_path =
        argc > 2 ? argv[2] : "/tmp/device_automation_task_system_cli.sqlite";

    try
    {
        auto graph = build_graph(load_tasks(workflow_path));

        device_automation::infrastructure::SqliteDatabase db;
        if (!db.open(db_path))
        {
            std::cerr << "failed to open database: " << db_path << '\n';
            return 2;
        }

        device_automation::infrastructure::EventBus event_bus;
        device_automation::scheduler::SimpleScheduler scheduler(db, event_bus);
        device_automation::application::WorkflowManager manager(scheduler);
        device_automation::scheduler::InlineExecutor executor;

        const bool ok =
            manager.submit_and_run(graph, [&executor](device_automation::domain::Task &task)
                                   { return executor.execute(task); });

        std::cout << "workflow=" << graph.name << " state=" << (ok ? "Completed" : "Failed")
                  << " tasks=" << graph.tasks().size()
                  << " audits=" << scheduler.audit_records().size() << '\n';
        return ok ? 0 : 3;
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
