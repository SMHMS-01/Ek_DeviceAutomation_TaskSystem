# Device Automation Task System — Master Development Roadmap v1.0

> **目标**: 从设计文档到生产级实现的完整开发路线图。本文档定义分阶段实现计划、验收测试标准、工程规范、代码组织、构建系统配置。

---

## 📋 目录

1. [开发阶段规划](#1-开发阶段规划)
2. [文件组织架构](#2-文件组织架构)
3. [命名约定与代码风格](#3-命名约定与代码风格)
4. [头文件包含规则](#4-头文件包含规则)
5. [测试策略](#5-测试策略)
6. [构建与工具链](#6-构建与工具链)
7. [持续集成检查](#7-持续集成检查)
8. [版本管理与发布](#8-版本管理与发布)

---

## 1. 开发阶段规划

### 1.1 整体时间线

```
Phase 1: Domain Layer (Weeks 1-3)
  ├─ Task/TaskGraph/FSM 核心模型
  ├─ Event 系统基础
  └─ Type 定义 & Serialization

Phase 2: Infrastructure Layer (Weeks 4-6)
  ├─ Database 持久化层
  ├─ EventBus 实现
  ├─ DeviceAbstraction 设备接口
  └─ PluginLoader 插件系统

Phase 3: Scheduler & Executors (Weeks 7-10)
  ├─ DAG Scheduler 核心引擎
  ├─ ExecutorPool 管理
  ├─ ThreadPoolExecutor
  ├─ CoroutineExecutor
  ├─ DeviceExecutor
  └─ SchedulingPolicy 策略

Phase 4: Fault Tolerance (Weeks 11-13)
  ├─ CheckPoint 保存与恢复
  ├─ RetryPolicy 重试机制
  ├─ CircuitBreaker 断路器
  └─ RollbackManager 回滚管理

Phase 5: Application Layer Services (Weeks 14-16)
  ├─ WorkflowManager
  ├─ ManualInterventionService
  ├─ AuditService & AuditEngine
  └─ HealthMonitor & WatchDog

Phase 6: Integration & System Tests (Weeks 17-19)
  ├─ 端到端工作流测试
  ├─ 设备模拟器集成
  ├─ 压力测试与 Benchmark
  └─ 性能分析优化

Phase 7: Documentation & Release (Weeks 20-22)
  ├─ API 文档生成
  ├─ 开发者指南
  ├─ 部署指南
  └─ v1.0.0 发布
```

### 1.2 Phase 1: Domain Layer 核心模型

**目标**: 实现不依赖外部库的纯领域模型

| 任务 | 文件 | 验收标准 |
|------|------|---------|
| **Task/TaskId 定义** | `src/domain/Task.h/.cpp` | 支持UUID、序列化、状态验证；单测覆盖100% |
| **TaskState FSM** | `src/domain/TaskStateMachine.h/.cpp` | 验证所有合法/非法状态转换；单测20+个状态转换场景 |
| **Priority 枚举** | `src/domain/Priority.h` | 支持比较、序列化 |
| **TaskGraph 有向图** | `src/domain/TaskGraph.h/.cpp` | 拓扑排序、关键路径分析；单测含循环检测 |
| **Event 基类** | `src/domain/Event.h` | 事件类型体系；支持序列化、时间戳 |
| **CheckPoint 结构** | `src/domain/CheckPoint.h/.cpp` | 阶段快照、JSON 序列化；单测恢复流程 |
| **RetryPolicy 结构** | `src/domain/RetryPolicy.h/.cpp` | 退避策略、错误分类；单测退避时间计算 |
| **Timestamp/ID 工具** | `src/domain/Types.h` | UUID 生成、时间戳、类型别名 |

**验收测试**:
- ✅ 单元测试：100+ 用例，覆盖所有状态转换、序列化、边界条件
- ✅ 无外部依赖（除标准库和 nlohmann::json）
- ✅ 代码覆盖率 > 95%

**交付物**:
- `src/domain/` 完整目录，所有 `.h` + `.cpp`
- `tests/unit/domain/` 单元测试套件

---

### 1.3 Phase 2: Infrastructure Layer 基础设施

**目标**: 建立持久化、事件、设备接口基础

| 任务 | 文件 | 验收标准 |
|------|------|---------|
| **Database 接口** | `src/infrastructure/IDatabase.h` | 纯虚接口，支持事务、查询 |
| **SQLite 实现** | `src/infrastructure/SqliteDatabase.h/.cpp` | 关系持久化，支持迁移；单测含并发 |
| **EventBus** | `src/infrastructure/EventBus.h/.cpp` | 发布-订阅，异步分发；单测并发订阅者 |
| **IDevice 接口** | `src/infrastructure/IDevice.h` | 抽象设备协议：connect/send/recv/disconnect |
| **MockDevice 实现** | `src/infrastructure/MockDevice.h/.cpp` | 测试用模拟设备 |
| **PluginLoader** | `src/infrastructure/PluginLoader.h/.cpp` | 动态加载 `.so`/.dll 插件 |
| **WatchDog** | `src/infrastructure/WatchDog.h/.cpp` | 进程健康检查、看门狗计时器 |
| **Logger** | `src/infrastructure/Logger.h/.cpp` | 日志系统（支持级别、输出流） |

**验收测试**:
- ✅ 数据库事务隔离单测；并发写入 1000+ 操作
- ✅ EventBus 异步分发 10,000+ 事件无丢失
- ✅ PluginLoader 加载自定义 `.so` 插件
- ✅ WatchDog 心跳检测误差 < 100ms
- ✅ 代码覆盖率 > 90%

**交付物**:
- `src/infrastructure/` 完整实现
- `tests/unit/infrastructure/` 单元测试
- `tests/fixtures/` 测试数据和模拟对象

---

### 1.4 Phase 3: 调度与执行引擎

**目标**: 实现 DAG 调度和多种执行器

| 任务 | 文件 | 验收标准 |
|------|------|---------|
| **IExecutor 接口** | `src/scheduler/IExecutor.h` | 纯虚接口：submit/cancel/pause/resume |
| **ThreadPoolExecutor** | `src/scheduler/ThreadPoolExecutor.h/.cpp` | 线程池 + 工作队列；单测 100 并发任务 |
| **CoroutineExecutor** | `src/scheduler/CoroutineExecutor.h/.cpp` | 协程实现（C++20 or Boost.Asio）；单测 1000 并发 I/O |
| **DeviceExecutor** | `src/scheduler/DeviceExecutor.h/.cpp` | 绑定单一设备，串行操作 |
| **ScriptExecutor** | `src/scheduler/ScriptExecutor.h/.cpp` | 脚本沙箱执行（Lua/Python 可选） |
| **ExecutorPool** | `src/scheduler/ExecutorPool.h/.cpp` | 执行器生命周期管理 |
| **SchedulingPolicy 接口** | `src/scheduler/SchedulingPolicy.h` | 纯虚接口：assign() |
| **RoundRobinPolicy** | `src/scheduler/RoundRobinPolicy.h/.cpp` | 轮询分配 |
| **PriorityFirstPolicy** | `src/scheduler/PriorityFirstPolicy.h/.cpp` | 优先级 + 等待时长 |
| **DeviceAffinityPolicy** | `src/scheduler/DeviceAffinityPolicy.h/.cpp` | 设备亲和性 |
| **LoadBalancePolicy** | `src/scheduler/LoadBalancePolicy.h/.cpp` | 负载均衡 |
| **Scheduler 核心** | `src/scheduler/Scheduler.h/.cpp` | DAG 调度、依赖解析、流量控制 |

**验收测试**:
- ✅ ThreadPoolExecutor: 100 并发任务，无死锁、无任务丢失
- ✅ CoroutineExecutor: 1000 并发 I/O，延迟 < 100ms
- ✅ Scheduler: DAG 依赖正确解析，拓扑排序无环
- ✅ 循环依赖检测单测（应拒绝）
- ✅ 优先级排序正确性单测
- ✅ RateLimiter 限流 1000 任务/秒
- ✅ 代码覆盖率 > 90%

**Benchmark**:
- 单一 Executor 吞吐: > 10,000 任务/秒
- 任务分配延迟: < 10ms
- DAG 拓扑排序 (1000 节点): < 50ms

**交付物**:
- `src/scheduler/` 完整实现
- `tests/unit/scheduler/` 单元测试
- `tests/benchmark/scheduler_benchmark.cpp` 性能基准

---

### 1.5 Phase 4: 容错与恢复

**目标**: 实现 Checkpoint、重试、回滚机制

| 任务 | 文件 | 验收标准 |
|------|------|---------|
| **CheckPointManager** | `src/faulttolerance/CheckPointManager.h/.cpp` | 保存/加载/恢复 CheckPoint |
| **RetryEngine** | `src/faulttolerance/RetryEngine.h/.cpp` | 重试策略、退避计算 |
| **CircuitBreaker** | `src/faulttolerance/CircuitBreaker.h/.cpp` | 熔断状态机：Closed→Open→HalfOpen |
| **RollbackManager** | `src/faulttolerance/RollbackManager.h/.cpp` | 回滚执行、设备清理 |
| **ErrorClassifier** | `src/faulttolerance/ErrorClassifier.h/.cpp` | 错误分类（可重试/不可重试） |

**验收测试**:
- ✅ CheckPoint 保存恢复：10,000+ 字节大对象正确序列化/反序列化
- ✅ RetryEngine 指数退避计算正确（base=500ms, max=30s）
- ✅ CircuitBreaker 状态转换正确；冷却期生效
- ✅ RollbackManager 调用设备清理接口
- ✅ 误恢复率 = 0（所有恢复点可重现）
- ✅ 代码覆盖率 > 95%

**Benchmark**:
- CheckPoint 序列化 10MB 对象: < 100ms
- 恢复 1000 个 CheckPoint: < 500ms
- CircuitBreaker 状态转换: < 1ms

**交付物**:
- `src/faulttolerance/` 完整实现
- `tests/unit/faulttolerance/` 单元测试
- `tests/benchmark/faulttolerance_benchmark.cpp`

---

### 1.6 Phase 5: 应用服务层

**目标**: 实现高级服务接口

| 任务 | 文件 | 验收标准 |
|------|------|---------|
| **WorkflowManager** | `src/application/WorkflowManager.h/.cpp` | 工作流生命周期管理 |
| **ManualInterventionService** | `src/application/ManualInterventionService.h/.cpp` | 暂停/恢复/强制完成等操作 |
| **AuditService** | `src/application/AuditService.h/.cpp` | 审计事件日志、查询接口 |
| **AuditEngine** | `src/application/AuditEngine.h/.cpp` | 事件持久化、时间序列分析 |
| **HealthMonitor** | `src/application/HealthMonitor.h/.cpp` | 系统健康指标收集 |

**验收测试**:
- ✅ WorkflowManager 支持 100+ 并发工作流
- ✅ ManualInterventionService 所有操作（pause/resume/cancel/retry/rollback）单测覆盖
- ✅ AuditService 记录所有操作，无丢失；查询延迟 < 100ms
- ✅ 代码覆盖率 > 90%

**交付物**:
- `src/application/` 完整实现
- `tests/unit/application/` 单元测试

---

### 1.7 Phase 6: 集成与系统测试

**目标**: 端到端工作流验证、性能优化

| 任务 | 文件 | 验收标准 |
|------|------|---------|
| **E2E 工作流测试** | `tests/integration/e2e_workflow_test.cpp` | 完整工作流：提交→调度→执行→完成；5+ 场景 |
| **设备模拟器** | `tests/fixtures/MockDeviceSimulator.h/.cpp` | 可配置的设备延迟、故障注入 |
| **压力测试** | `tests/stress/stress_test.cpp` | 10,000+ 任务工作流，验证无泄漏 |
| **故障恢复测试** | `tests/integration/failover_test.cpp` | 模拟崩溃恢复、数据完整性验证 |
| **性能 Benchmark** | `tests/benchmark/system_benchmark.cpp` | 端到端延迟、吞吐、资源占用 |

**验收标准**:
- ✅ E2E 测试 5+ 复杂场景通过，覆盖所有任务状态
- ✅ 压力测试 10,000 任务无内存泄漏、无死锁
- ✅ 故障恢复：模拟设备故障/网络中断，系统自动恢复率 > 99%
- ✅ 性能指标达标：
  - 端到端工作流延迟: < 1s（10 任务 DAG）
  - 吞吐: > 100 工作流/秒
  - 内存占用: < 500MB（1000 并发任务）

**交付物**:
- `tests/integration/` E2E 测试
- `tests/stress/` 压力测试
- `tests/benchmark/` 性能基准
- 性能报告 (`docs/performance_report.md`)

---

### 1.8 Phase 7: 文档与发布

**目标**: 完整文档、示例代码、v1.0.0 发布

| 任务 | 文件 | 验收标准 |
|------|------|---------|
| **API 文档（Doxygen）** | `docs/api/` | 所有公共类/函数完整 Doxygen 注释 |
| **开发者指南** | `docs/DEVELOPER_GUIDE.md` | 快速开始、常见场景示例代码 |
| **部署指南** | `docs/DEPLOYMENT_GUIDE.md` | 配置、初始化、故障排查 |
| **示例工作流** | `examples/` | 3+ 真实场景示例：设备控制、批处理、错误恢复 |
| **变更日志** | `CHANGELOG.md` | v1.0.0 所有特性、修复、已知限制 |

**验收标准**:
- ✅ Doxygen 构建无警告
- ✅ 开发者指南含 5+ 完整示例代码
- ✅ 部署指南覆盖所有组件配置
- ✅ 示例代码可编译、可运行

**交付物**:
- `docs/` 完整文档
- `examples/` 示例代码
- Release v1.0.0 标签 + GitHub Release 页面

---

## 2. 文件组织架构

### 2.1 目录树

```
device-automation-task-system/
├── CMakeLists.txt                 # 根 CMake 配置
├── .clang-format                  # 代码风格配置
├── .clang-tidy                    # 静态分析配置
├── .gitignore
│
├── src/                           # 源代码（严格分层）
│   ├── CMakeLists.txt
│   ├── domain/                    # Layer 0: 纯领域模型（无外部依赖）
│   │   ├── CMakeLists.txt
│   │   ├── Task.h
│   │   ├── Task.cpp
│   │   ├── TaskStateMachine.h
│   │   ├── TaskStateMachine.cpp
│   │   ├── TaskGraph.h
│   │   ├── TaskGraph.cpp
│   │   ├── Event.h
│   │   ├── CheckPoint.h
│   │   ├── RetryPolicy.h
│   │   ├── Priority.h
│   │   ├── Types.h                # UUID, Timestamp, 类型别名
│   │   └── ...
│   │
│   ├── infrastructure/            # Layer 1: 基础设施
│   │   ├── CMakeLists.txt
│   │   ├── IDatabase.h
│   │   ├── SqliteDatabase.h/.cpp
│   │   ├── EventBus.h/.cpp
│   │   ├── IDevice.h
│   │   ├── MockDevice.h/.cpp
│   │   ├── PluginLoader.h/.cpp
│   │   ├── WatchDog.h/.cpp
│   │   ├── Logger.h/.cpp
│   │   └── ...
│   │
│   ├── scheduler/                 # Layer 2: 调度与执行
│   │   ├── CMakeLists.txt
│   │   ├── IExecutor.h
│   │   ├── ThreadPoolExecutor.h/.cpp
│   │   ├── CoroutineExecutor.h/.cpp
│   │   ├── DeviceExecutor.h/.cpp
│   │   ├── ScriptExecutor.h/.cpp
│   │   ├── ExecutorPool.h/.cpp
│   │   ├── SchedulingPolicy.h
│   │   ├── RoundRobinPolicy.h/.cpp
│   │   ├── PriorityFirstPolicy.h/.cpp
│   │   ├── DeviceAffinityPolicy.h/.cpp
│   │   ├── LoadBalancePolicy.h/.cpp
│   │   ├── Scheduler.h/.cpp
│   │   ├── RateLimiter.h/.cpp
│   │   └── ...
│   │
│   ├── faulttolerance/            # Layer 3: 容错
│   │   ├── CMakeLists.txt
│   │   ├── CheckPointManager.h/.cpp
│   │   ├── RetryEngine.h/.cpp
│   │   ├── CircuitBreaker.h/.cpp
│   │   ├── RollbackManager.h/.cpp
│   │   ├── ErrorClassifier.h/.cpp
│   │   └── ...
│   │
│   └── application/               # Layer 4: 应用服务
│       ├── CMakeLists.txt
│       ├── WorkflowManager.h/.cpp
│       ├── ManualInterventionService.h/.cpp
│       ├── AuditService.h/.cpp
│       ├── AuditEngine.h/.cpp
│       ├── HealthMonitor.h/.cpp
│       └── ...
│
├── tests/                         # 测试代码（镜像 src 结构）
│   ├── CMakeLists.txt
│   ├── unit/                      # 单元测试
│   │   ├── CMakeLists.txt
│   │   ├── domain/
│   │   │   ├── test_task.cpp
│   │   │   ├── test_task_state_machine.cpp
│   │   │   ├── test_task_graph.cpp
│   │   │   └── ...
│   │   ├── infrastructure/
│   │   │   ├── test_database.cpp
│   │   │   ├── test_event_bus.cpp
│   │   │   └── ...
│   │   ├── scheduler/
│   │   ├── faulttolerance/
│   │   └── application/
│   │
│   ├── integration/                # 集成测试
│   │   ├── CMakeLists.txt
│   │   ├── test_e2e_workflow.cpp
│   │   ├── test_failover.cpp
│   │   └── ...
│   │
│   ├── stress/                    # 压力测试
│   │   ├── CMakeLists.txt
│   │   └── stress_test.cpp
│   │
│   ├── benchmark/                 # 性能基准测试
│   │   ├── CMakeLists.txt
│   │   ├── scheduler_benchmark.cpp
│   │   ├── faulttolerance_benchmark.cpp
│   │   ├── system_benchmark.cpp
│   │   └── ...
│   │
│   └── fixtures/                  # 测试数据/模拟对象
│       ├── CMakeLists.txt
│       ├── MockDeviceSimulator.h/.cpp
│       ├── TestDataFactory.h/.cpp
│       └── ...
│
├── docs/                          # 文档
│   ├── api/                       # Doxygen API 文档
│   ├── DEVELOPER_GUIDE.md
│   ├── DEPLOYMENT_GUIDE.md
│   ├── architecture.md
│   ├── performance_report.md
│   └── images/                    # 架构图、FSM 图等
│
├── examples/                      # 示例代码
│   ├── example_device_control.cpp
│   ├── example_batch_processing.cpp
│   ├── example_error_recovery.cpp
│   └── ...
│
├── third_party/                   # 第三方库（子模块或直接引入）
│   ├── json/                      # nlohmann/json
│   ├── googletest/
│   ├── benchmark/
│   └── ...
│
├── build/                         # 构建输出（Git 忽略）
├── MASTER_ROADMAP.md              # 本文件
├── README.md
├── AGENTS.md
└── CHANGELOG.md
```

### 2.2 分层隔离规则

```
Layer 0 (Domain)
  └─ 仅依赖标准库 + nlohmann::json

Layer 1 (Infrastructure)
  ├─ 依赖 Layer 0
  └─ 可依赖外部库（SQLite, Boost, etc.）

Layer 2 (Scheduler)
  ├─ 依赖 Layer 0, 1
  └─ 禁止依赖 Layer 3, 4

Layer 3 (FaultTolerance)
  ├─ 依赖 Layer 0, 1, 2
  └─ 禁止依赖 Layer 4

Layer 4 (Application)
  └─ 依赖 Layer 0, 1, 2, 3

【CI 检查】违反此规则的 PR 自动拒绝
```

---

## 3. 命名约定与代码风格

### 3.1 命名约定

| 元素 | 约定 | 示例 |
|------|------|------|
| **文件名** | 模块名 = 文件名（PascalCase） | `TaskStateMachine.h`, `SqliteDatabase.cpp` |
| **类名** | PascalCase | `class TaskGraph`, `class Scheduler` |
| **函数名** | camelCase（公开）, snake_case（私有） | `void submit(Task t)`, `void schedule_loop_()` |
| **常量** | UPPER_SNAKE_CASE | `const int MAX_RETRIES = 3;` |
| **成员变量** | snake_case_（私有），public 无后缀 | `int max_retries_`, `std::string task_id` |
| **枚举值** | PascalCase | `enum State { Pending, Ready, Running }` |
| **命名空间** | snake_case（小写） | `namespace device_automation { }` |
| **宏** | UPPER_SNAKE_CASE | `#define LOG_DEBUG(msg) ...` |
| **模板参数** | 单字母或 PascalCase | `template<typename T>`, `template<class TaskType>` |

**禁止事项**:
- ❌ 匈牙利命名法：`m_taskId`, `pTask`, `szBuffer`
- ❌ 缩写：`sch` → `scheduler`, `evt` → `event`
- ❌ 歧义前缀：`I` 接口前缀不必（含义不清）
- ❌ 数字作为名字：`task1`, `var2`

### 3.2 代码风格（clang-format）

**`.clang-format` 配置**:

```yaml
BasedOnStyle: LLVM
IndentWidth: 4
UseTab: Never
ColumnLimit: 100
AccessModifierOffset: -4
BreakBeforeBraces: Allman
PointerAlignment: Right
SpaceAfterCStyleCast: true
SpaceBeforeCtorInitializerColon: true
AlignEscapedNewlines: Right
AllowShortFunctionsOnASingleLine: None
AllowShortLambdasOnASingleLine: All
AllowShortBlocksOnASingleLine: Never
AllowShortCaseLabelsOnASingleLine: false
IncludeBlocks: Regroup
IncludeIsMainRegex: "(Test)?$"
```

**示例代码**:

```cpp
#include <memory>
#include <vector>

#include "domain/Task.h"
#include "infrastructure/EventBus.h"

namespace device_automation::scheduler {

class Scheduler
{
public:
    Scheduler(std::shared_ptr<EventBus> event_bus, int thread_count);
    ~Scheduler();

    WorkflowId submit(const TaskGraph& graph);
    bool pause(TaskId task_id, const std::string& reason);

private:
    void dispatch_loop_();
    bool dependencies_met_(const Task& task) const;
    ExecutorId select_executor_(const Task& task);

    std::shared_ptr<EventBus> event_bus_;
    std::vector<std::shared_ptr<IExecutor>> executors_;
    int thread_count_;
};

} // namespace device_automation::scheduler
```

### 3.3 注释规范

**使用 Doxygen 风格注释**:

```cpp
/**
 * @brief 提交一个工作流到调度器
 * @param graph 任务图（有向无环图，已验证无循环）
 * @param opts 提交选项（优先级、超时等）
 * @return 工作流 ID，全局唯一
 * @throws std::invalid_argument 如果 graph 包含循环依赖
 * @note 此操作是原子的，失败时不会产生部分效果
 * @see TaskGraph::topological_sort()
 */
WorkflowId submit(const TaskGraph& graph, const SubmitOptions& opts);
```

**禁止注释**:
- ❌ 冗余注释：`i++; // i 自增 1`
- ❌ 过期注释：代码改了，注释没改
- ❌ TODO/FIXME：用 GitHub Issue 替代

---

## 4. 头文件包含规则

### 4.1 包含顺序

```cpp
// 1. 本文件对应的头文件（如果这是 .cpp）
#include "scheduler/Scheduler.h"

// 2. C++ 标准库（字母序）
#include <algorithm>
#include <memory>
#include <vector>

// 3. 第三方库（字母序）
#include <nlohmann/json.hpp>
#include <sqlite3.h>

// 4. 项目内头文件（按层级 + 字母序）
#include "domain/Task.h"
#include "domain/Types.h"
#include "infrastructure/EventBus.h"
#include "infrastructure/IDatabase.h"
```

### 4.2 头文件防卫

所有 `.h` 文件必须使用现代 `#pragma once`（已取代 include guard）:

```cpp
#pragma once

#include <memory>

namespace device_automation::domain {

class Task
{
    // ...
};

} // namespace device_automation::domain
```

### 4.3 分层包含规则（自动 CI 检查）

| 规则 | 检查内容 |
|------|---------|
| **Layer N 禁止包含 Layer N+1+** | `src/scheduler/` 禁止 `#include` 来自 `src/application/` 的头文件 |
| **允许跨层向下** | `src/application/` 可以 `#include` 来自 `src/scheduler/` |
| **允许同层包含** | `src/scheduler/` 内可互相 `#include` |
| **禁止循环包含** | A.h 包含 B.h，B.h 禁止包含 A.h（即便间接） |

### 4.4 CI 脚本检查

**脚本**: `scripts/check_includes.py`

```python
#!/usr/bin/env python3
"""
检查头文件包含规则违反
- 分层隔离：Layer N 禁止包含 Layer N+1 或更高
- 循环依赖检测
"""

import os
import re
from collections import defaultdict, deque

LAYERS = {
    0: "src/domain",
    1: "src/infrastructure",
    2: "src/scheduler",
    3: "src/faulttolerance",
    4: "src/application",
}

def get_layer(file_path):
    """获取文件所在的层"""
    for layer, prefix in LAYERS.items():
        if file_path.startswith(prefix):
            return layer
    return None

def extract_includes(file_path):
    """提取文件的所有 #include"""
    includes = []
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        for line in f:
            match = re.match(r'#include\s+[<"]([^>"]+)[>"]', line)
            if match:
                includes.append(match.group(1))
    return includes

def check_layering(file_path):
    """检查文件是否违反分层规则"""
    violations = []
    source_layer = get_layer(file_path)
    if source_layer is None:
        return violations
    
    includes = extract_includes(file_path)
    for inc in includes:
        # 只检查项目内包含（不检查 <> 系统库）
        if inc.startswith('src/'):
            target_layer = get_layer(inc)
            if target_layer is not None and target_layer > source_layer:
                violations.append({
                    'file': file_path,
                    'include': inc,
                    'source_layer': source_layer,
                    'target_layer': target_layer,
                    'message': f"Layer {source_layer} cannot include Layer {target_layer}"
                })
    
    return violations

def check_circular_includes():
    """检查循环依赖"""
    # 构建包含图
    include_graph = defaultdict(set)
    for root, dirs, files in os.walk('src'):
        for file in files:
            if file.endswith('.h'):
                file_path = os.path.join(root, file)
                includes = extract_includes(file_path)
                for inc in includes:
                    if inc.startswith('src/'):
                        include_graph[file_path].add(inc)
    
    # DFS 检测循环
    cycles = []
    visited = set()
    rec_stack = set()
    
    def dfs(node, path):
        visited.add(node)
        rec_stack.add(node)
        
        for neighbor in include_graph.get(node, []):
            if neighbor not in visited:
                if dfs(neighbor, path + [neighbor]):
                    return True
            elif neighbor in rec_stack:
                cycles.append(path + [neighbor])
                return True
        
        rec_stack.remove(node)
        return False
    
    for node in include_graph:
        if node not in visited:
            dfs(node, [node])
    
    return cycles

if __name__ == '__main__':
    violations = []
    
    # 检查分层
    for root, dirs, files in os.walk('src'):
        for file in files:
            if file.endswith('.h') or file.endswith('.cpp'):
                file_path = os.path.join(root, file)
                violations.extend(check_layering(file_path))
    
    # 检查循环
    cycles = check_circular_includes()
    
    # 输出结果
    if violations or cycles:
        print("❌ Include rule violations found:")
        for v in violations:
            print(f"  {v['file']}: {v['message']}")
            print(f"    includes: {v['include']}")
        for cycle in cycles:
            print(f"  Circular: {' -> '.join(cycle)}")
        exit(1)
    else:
        print("✅ All include rules pass")
        exit(0)
```

**使用**:

```bash
# 手动检查
python3 scripts/check_includes.py

# CI 集成（GitHub Actions）
- name: Check Include Rules
  run: python3 scripts/check_includes.py
```

---

## 5. 测试策略

### 5.1 测试框架与工具

| 工具 | 用途 | 说明 |
|------|------|------|
| **Google Test** | 单元测试框架 | C++ 标准单测框架 |
| **Google Benchmark** | 性能基准测试 | 微基准测试，支持统计分析 |
| **LCOV** | 代码覆盖率 | 生成 HTML 报告 |
| **Valgrind** | 内存检查 | 检测泄漏、溢出（可选，仅 Linux） |
| **AddressSanitizer** | 运行时错误检测 | 编译标志 `-fsanitize=address` |

### 5.2 单元测试标准

**测试文件命名**: `test_{module_name}.cpp`

**示例**:

```cpp
#include <gtest/gtest.h>

#include "domain/Task.h"

namespace device_automation::domain {

class TaskTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // 测试前初始化
    }

    void TearDown() override
    {
        // 测试后清理
    }

    Task create_sample_task()
    {
        return Task{
            .id = TaskId::generate(),
            .name = "sample_task",
            .kind = TaskKind::Atomic,
            .priority = Priority::Normal,
        };
    }
};

TEST_F(TaskTest, TaskIdIsUnique)
{
    auto task1 = create_sample_task();
    auto task2 = create_sample_task();
    EXPECT_NE(task1.id, task2.id);
}

TEST_F(TaskTest, TaskSerializationRoundTrip)
{
    auto original = create_sample_task();
    auto json = original.to_json();
    auto restored = Task::from_json(json);
    EXPECT_EQ(original.id, restored.id);
    EXPECT_EQ(original.name, restored.name);
}

TEST_F(TaskTest, InvalidStateTransitionThrows)
{
    auto task = create_sample_task();
    task.state = TaskState::Running;
    
    // Running 状态不能转换到 Ready
    EXPECT_THROW(task.transition_to(TaskState::Ready), std::invalid_argument);
}

} // namespace device_automation::domain
```

**测试覆盖率要求**:

| 代码类型 | 最低覆盖率 |
|---------|-----------|
| Domain Layer | 95%+ |
| Infrastructure Layer | 90%+ |
| Scheduler | 90%+ |
| FaultTolerance | 95%+ |
| Application | 85%+ |

**生成覆盖率报告**:

```bash
# 编译时添加覆盖率标志
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
make

# 运行测试
make test

# 生成报告
lcov --directory . --capture --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/third_party/*' --output-file coverage.info
genhtml coverage.info --output-directory coverage_report

# 查看
open coverage_report/index.html
```

### 5.3 集成测试与 E2E 测试

**E2E 工作流测试示例**:

```cpp
#include <gtest/gtest.h>

#include "application/WorkflowManager.h"
#include "scheduler/Scheduler.h"
#include "infrastructure/MockDevice.h"

namespace device_automation::integration {

class E2EWorkflowTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        event_bus_ = std::make_shared<EventBus>();
        device_pool_ = std::make_shared<DevicePool>();
        scheduler_ = std::make_shared<Scheduler>(event_bus_, 4);
        workflow_manager_ = std::make_unique<WorkflowManager>(scheduler_, event_bus_);
    }

    std::shared_ptr<EventBus> event_bus_;
    std::shared_ptr<DevicePool> device_pool_;
    std::shared_ptr<Scheduler> scheduler_;
    std::unique_ptr<WorkflowManager> workflow_manager_;
};

TEST_F(E2EWorkflowTest, SimpleSequentialWorkflow)
{
    // 创建简单的顺序工作流：Task1 → Task2 → Task3
    TaskGraph graph;
    auto task1 = domain::Task{.id = TaskId::generate(), .name = "task1"};
    auto task2 = domain::Task{.id = TaskId::generate(), .name = "task2", .dependencies = {task1.id}};
    auto task3 = domain::Task{.id = TaskId::generate(), .name = "task3", .dependencies = {task2.id}};
    
    graph.tasks = {
        {task1.id, task1},
        {task2.id, task2},
        {task3.id, task3},
    };

    // 提交工作流
    auto workflow_id = workflow_manager_->submit(graph);
    
    // 等待完成（最多 10 秒）
    EXPECT_TRUE(workflow_manager_->wait_completion(workflow_id, std::chrono::seconds(10)));
    
    // 验证最终状态
    auto status = workflow_manager_->query(workflow_id);
    EXPECT_EQ(status.state, GraphState::Completed);
    EXPECT_EQ(status.task_states[task1.id], TaskState::Completed);
    EXPECT_EQ(status.task_states[task2.id], TaskState::Completed);
    EXPECT_EQ(status.task_states[task3.id], TaskState::Completed);
}

TEST_F(E2EWorkflowTest, ParallelWorkflowWithRetry)
{
    // 创建并行工作流，Task1 失败后重试
    TaskGraph graph;
    auto task1 = domain::Task{
        .id = TaskId::generate(),
        .name = "flaky_task",
        .retry_policy = domain::RetryPolicy{.max_attempts = 3, .auto_retry = true},
    };
    auto task2 = domain::Task{.id = TaskId::generate(), .name = "parallel_task"};
    
    // ...setup...

    auto workflow_id = workflow_manager_->submit(graph);
    
    // 验证重试发生
    EXPECT_GE(workflow_manager_->query(workflow_id).retry_count[task1.id], 1);
}

} // namespace device_automation::integration
```

### 5.4 性能 Benchmark

**示例 Benchmark**:

```cpp
#include <benchmark/benchmark.h>

#include "scheduler/Scheduler.h"
#include "domain/TaskGraph.h"

namespace device_automation::benchmark {

class SchedulerBenchmark : public ::benchmark::Fixture
{
public:
    void SetUp(const ::benchmark::State&) override
    {
        event_bus_ = std::make_shared<EventBus>();
        scheduler_ = std::make_shared<Scheduler>(event_bus_, 4);
    }

    void TearDown(const ::benchmark::State&) override
    {
        scheduler_->shutdown();
    }

    std::shared_ptr<EventBus> event_bus_;
    std::shared_ptr<Scheduler> scheduler_;
};

// 基准：单任务提交延迟
BENCHMARK_F(SchedulerBenchmark, SingleTaskSubmit)(benchmark::State& state)
{
    domain::TaskGraph graph;
    auto task = domain::Task{.id = TaskId::generate(), .name = "simple_task"};
    graph.tasks = {{task.id, task}};

    for (auto _ : state) {
        auto workflow_id = scheduler_->submit(graph);
        benchmark::DoNotOptimize(workflow_id);
    }
}

// 基准：100 个无依赖任务的调度
BENCHMARK_F(SchedulerBenchmark, HundredIndependentTasks)(benchmark::State& state)
{
    for (auto _ : state) {
        domain::TaskGraph graph;
        for (int i = 0; i < 100; ++i) {
            auto task = domain::Task{.id = TaskId::generate(), .name = std::string("task_") + std::to_string(i)};
            graph.tasks.emplace(task.id, task);
        }
        auto workflow_id = scheduler_->submit(graph);
        benchmark::DoNotOptimize(workflow_id);
    }
}

} // namespace device_automation::benchmark
```

**运行 Benchmark**:

```bash
./build/tests/benchmark/scheduler_benchmark --benchmark_out=results.json --benchmark_out_format=json
```

---

## 6. 构建与工具链

### 6.1 CMake 配置

**根 `CMakeLists.txt`**:

```cmake
cmake_minimum_required(VERSION 3.20)
project(device_automation_task_system
    VERSION 1.0.0
    LANGUAGES CXX
    DESCRIPTION "Sophisticated task management framework for device automation"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
add_compile_options(-Wall -Wextra -Wpedantic -Werror)

# 调试/优化
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-O0 -g3)
    add_compile_options(-fsanitize=address -fsanitize=undefined)
    add_link_options(-fsanitize=address -fsanitize=undefined)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -DNDEBUG)
endif()

# 覆盖率支持
option(ENABLE_COVERAGE "Enable code coverage" OFF)
if(ENABLE_COVERAGE)
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

# 找到依赖
find_package(SQLite3 REQUIRED)
find_package(nlohmann_json REQUIRED)

# 可选依赖
find_package(GTest CONFIG)
find_package(benchmark CONFIG)

# 源代码
add_subdirectory(src)

# 测试
if(GTest_FOUND)
    enable_testing()
    add_subdirectory(tests)
endif()

# 文档（Doxygen）
find_package(Doxygen)
if(DOXYGEN_FOUND)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
```

**层级 `CMakeLists.txt` 示例** (`src/domain/CMakeLists.txt`):

```cmake
add_library(device_automation_domain
    Task.cpp
    TaskStateMachine.cpp
    TaskGraph.cpp
    Event.cpp
    CheckPoint.cpp
    RetryPolicy.cpp
    Types.cpp
)

target_include_directories(device_automation_domain
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(device_automation_domain
    PUBLIC nlohmann_json::nlohmann_json
)

# 安装
install(TARGETS device_automation_domain
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY .
    DESTINATION include/device_automation/domain
    FILES_MATCHING PATTERN "*.h"
)
```

### 6.2 构建脚本

**`scripts/build.sh`**:

```bash
#!/bin/bash
set -e

mkdir -p build
cd build

# Debug 构建
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
make -j$(nproc)

# 运行测试
ctest --output-on-failure

# 生成覆盖率
if command -v lcov &> /dev/null; then
    lcov --directory . --capture --output-file coverage.info
    lcov --remove coverage.info '/usr/*' '*/third_party/*' --output-file coverage.info
    genhtml coverage.info --output-directory coverage_report
    echo "Coverage report: $(pwd)/coverage_report/index.html"
fi
```

### 6.3 工具链版本要求

| 工具 | 最低版本 | 推荐版本 |
|------|---------|---------|
| GCC / Clang | 11 / 13 | 13+ / 18+ |
| CMake | 3.20 | 3.25+ |
| C++ 标准 | C++20 | C++20/23 |

**检查工具版本**:

```bash
cmake --version
g++ --version
clang++ --version
```

---

## 7. 持续集成检查

### 7.1 CI 流程 (GitHub Actions)

**`.github/workflows/ci.yml`**:

```yaml
name: CI

on:
  push:
    branches: [main, develop]
  pull_request:
    branches: [main, develop]

jobs:
  build-and-test:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        compiler: [gcc-13, clang-18]
        build_type: [Debug, Release]

    steps:
      - uses: actions/checkout@v3

      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y \
            build-essential cmake ninja-build \
            libsqlite3-dev nlohmann-json3-dev \
            google-benchmark-dev googletest-dev \
            clang-format clang-tidy lcov

      - name: Format check
        run: |
          find src tests -name '*.h' -o -name '*.cpp' | xargs clang-format -i
          git diff --exit-code

      - name: Static analysis
        run: |
          python3 scripts/check_includes.py
          clang-tidy src/**/*.cpp -- -std=c++20

      - name: Build
        run: |
          mkdir build
          cd build
          cmake -DCMAKE_BUILD_TYPE=${{ matrix.build_type }} \
                -DENABLE_COVERAGE=ON \
                -DCMAKE_CXX_COMPILER=${{ matrix.compiler }} ..
          make -j$(nproc)

      - name: Test
        run: |
          cd build
          ctest --output-on-failure --verbose

      - name: Coverage report
        if: matrix.compiler == 'gcc-13' && matrix.build_type == 'Debug'
        run: |
          cd build
          lcov --directory . --capture --output-file coverage.info
          lcov --remove coverage.info '/usr/*' '*/third_party/*' --output-file coverage.info
          lcov --list coverage.info

      - name: Upload coverage
        if: matrix.compiler == 'gcc-13' && matrix.build_type == 'Debug'
        uses: codecov/codecov-action@v3
        with:
          files: ./build/coverage.info
```

### 7.2 代码审查检查清单

所有 PR 必须通过以下检查：

- ✅ CI 流程全部通过（编译、测试、覆盖率）
- ✅ 代码覆盖率增加或保持 > 阈值
- ✅ 没有分层包含违反（脚本检查）
- ✅ 代码风格符合 clang-format 规则
- ✅ 没有新的 clang-tidy 警告
- ✅ 所有新公共 API 有 Doxygen 注释
- ✅ 对应层级有单元测试（覆盖新代码）
- ✅ 如有 API 变更，必须更新 CHANGELOG.md

---

## 8. 版本管理与发布

### 8.1 版本策略（Semantic Versioning）

格式: `MAJOR.MINOR.PATCH[-prerelease]`

- **MAJOR**: 不兼容的 API 变更
- **MINOR**: 向后兼容的新特性
- **PATCH**: 向后兼容的 bug 修复
- **Prerelease**: 例 `1.0.0-alpha`, `1.0.0-rc.1`

### 8.2 分支策略

| 分支 | 用途 | 版本 |
|------|------|------|
| `main` | 发布分支（生产） | v1.0.0, v1.1.0, ... |
| `develop` | 集成分支 | v1.x.x-dev |
| `feature/*` | 特性分支 | N/A |
| `bugfix/*` | 修复分支 | N/A |
| `release/*` | 发布准备 | v1.x.0-rc.x |

### 8.3 发布流程

**发布前检查清单**:

1. 更新 `CHANGELOG.md`
2. 更新版本号：
   - `CMakeLists.txt`: `project(...VERSION x.x.x...)`
   - `src/version.h`: `#define VERSION "x.x.x"`
3. 所有测试通过，覆盖率达标
4. 创建 Release PR（从 `develop` → `main`）
5. 代码审查通过
6. 合并并创建标签

**发布标签格式**:

```bash
# 发布
git tag -a v1.0.0 -m "Release v1.0.0 - Initial stable release"
git push origin v1.0.0

# 生成 GitHub Release (自动或手动)
```

### 8.4 变更日志 (CHANGELOG.md)

```markdown
# Changelog

All notable changes to this project will be documented in this file.

## [1.0.0] - 2026-MM-DD (Design Phase Release)

### Added
- Domain layer: Task, TaskGraph, FSM state machine
- Infrastructure: Database, EventBus, DeviceAbstraction
- Scheduler: DAG scheduling engine with pluggable policies
- FaultTolerance: Checkpoint, Retry, CircuitBreaker, Rollback
- Application: WorkflowManager, ManualInterventionService, AuditService
- Comprehensive unit tests (1000+ test cases)
- Performance benchmarks for all major components

### Changed
- (First release - N/A)

### Fixed
- (First release - N/A)

### Known Limitations
- UI Layer not yet implemented
- Cloud synchronization not implemented
- Plugin security relies on process isolation (no sandbox)

### Testing
- Unit test coverage: > 90% per layer
- Stress tested: 10,000+ concurrent tasks
- Performance: 100+ workflows/second throughput

---

## [Unreleased]

### Added
- (Next phase features)
```

---

## 附录：快速参考

### 构建 & 测试

```bash
# 初始化
git clone <repo>
cd device-automation-task-system
mkdir build && cd build

# 调试构建 + 测试
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
make -j$(nproc)
ctest --output-on-failure

# 发布构建
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# 覆盖率报告
./scripts/generate_coverage_report.sh
```

### 代码检查

```bash
# 格式化
clang-format -i $(find src tests -name '*.cpp' -o -name '*.h')

# 包含规则检查
python3 scripts/check_includes.py

# 静态分析
clang-tidy src/**/*.cpp -- -std=c++20

# 内存检查（调试构建）
valgrind --leak-check=full ./build/tests/unit/*_test
```

### PR 工作流

```bash
# 新特性分支
git checkout -b feature/my-feature develop

# 开发、提交、推送
git add .
git commit -m "feat: description"
git push origin feature/my-feature

# 创建 PR 到 develop
# GitHub UI: Open PR, describe changes

# 审查后合并到 develop
git checkout develop
git pull
git merge --no-ff feature/my-feature
git push origin develop

# 发布时从 develop 创建 Release PR 到 main
```

---

**作者**: Device Automation Task System 团队  
**最后更新**: May 6, 2026  
**版本**: 1.0.0-design
