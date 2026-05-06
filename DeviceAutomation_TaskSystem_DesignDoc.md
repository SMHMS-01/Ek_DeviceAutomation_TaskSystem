# 设备自动化任务系统 — 设计书 v1.0

> **文档定位**：本文档描述一套面向实验室/工业设备自动化场景的通用任务管理框架，重点覆盖任务抽象、调度、执行、容错、审计与扩展机制。

---

## 目录

1. [系统目标与约束](#1-系统目标与约束)
2. [整体架构](#2-整体架构)
3. [任务模型：抽象与状态机](#3-任务模型抽象与状态机)
4. [DAG 调度引擎](#4-dag-调度引擎)
5. [执行器层](#5-执行器层)
6. [容错、干预与恢复机制](#6-容错干预与恢复机制)
7. [审计日志与持久化](#7-审计日志与持久化)
8. [设备抽象与通信协议](#8-设备抽象与通信协议)
9. [插件系统](#9-插件系统)
10. [事件总线与内部通信](#10-事件总线与内部通信)
11. [健康监控与看门狗](#11-健康监控与看门狗)
12. [配置与版本管理](#12-配置与版本管理)
13. [安全与权限](#13-安全与权限)
14. [开源库选型](#14-开源库选型)
15. [补充项：你的盲区](#15-补充项你的盲区)

---

## 1. 系统目标与约束

### 1.1 目标

| 目标 | 说明 |
|------|------|
| **高可靠性** | 样本成本极高，一个任务失败不能导致整批数据丢失 |
| **可追溯** | 每个样本从入队到完成/失败，全程可审计 |
| **可扩展** | 支持动态添加设备、协议和业务流程 |
| **可干预** | 人工可以在任意阶段安全暂停、恢复、回滚 |
| **可观测** | 实时监控系统状态、任务进度、资源使用率 |

### 1.2 核心约束

- **单机优先**：优先保证本地自治能力，云端为辅助手段
- **实时响应**：设备控制指令延迟 < 100ms（非硬实时，但需要可预期）
- **数据持久化**：任务状态和审计日志必须持久化到磁盘，避免软件崩溃导致数据丢失

---

## 2. 整体架构

```
┌────────────────────────────────────────────────────────────────┐
│                        用户界面层 (UI Layer)                    │
│          任务编辑器 │ 实时监控面板 │ 干预控制台 │ 审计查询        │
└────────────────────────────────┬───────────────────────────────┘
                                 │ Command / Query
┌────────────────────────────────▼───────────────────────────────┐
│                      应用服务层 (Application Layer)              │
│   WorkflowManager │ ManualInterventionService │ AuditService   │
└──────┬──────────────────────┬───────────────────────┬──────────┘
       │                      │                       │
┌──────▼──────┐   ┌───────────▼──────────┐  ┌────────▼─────────┐
│  DAG调度引擎  │   │   执行器池(Executor) │  │  审计日志引擎     │
│  (Scheduler) │   │   线程池 / 协程池    │  │  (Audit Engine)  │
└──────┬──────┘   └───────────┬──────────┘  └────────┬─────────┘
       │                      │                       │
┌──────▼──────────────────────▼───────────────────────▼─────────┐
│                      核心领域层 (Domain Layer)                   │
│    Task │ TaskGraph │ FSM │ CheckPoint │ RetryPolicy │ Event   │
└──────────────────────────────┬─────────────────────────────────┘
                               │
┌──────────────────────────────▼─────────────────────────────────┐
│                      基础设施层 (Infra Layer)                    │
│  DeviceAbstraction │ PluginLoader │ EventBus │ DB │ WatchDog   │
└────────────────────────────────────────────────────────────────┘
```

---

## 3. 任务模型：抽象与状态机

### 3.1 Task 数据结构

```cpp
// 任务类型：原子任务 vs 复合任务（包含子任务）
enum class TaskKind {
    Atomic,    // 叶节点，直接对应设备操作
    Composite, // 包含子任务，负责编排
    Script,    // 用户脚本定义的任务（插件扩展）
};

// 任务优先级
enum class Priority { Critical = 0, High, Normal, Low, Background };

struct Task {
    // --- 身份 ---
    TaskId          id;           // UUID，全局唯一
    std::string     name;
    TaskKind        kind;
    Priority        priority;

    // --- DAG 关系 ---
    std::vector<TaskId> dependencies;  // 前置任务
    std::vector<TaskId> dependents;    // 后置任务

    // --- 执行参数 ---
    nlohmann::json  params;       // 序列化的业务参数
    DeviceId        target_device;// 目标设备（可为空，由调度器选择）

    // --- 状态 ---
    TaskState       state;        // FSM 当前状态

    // --- 容错 ---
    RetryPolicy     retry_policy;
    CheckPoint      last_checkpoint;

    // --- 时间戳（审计用）---
    Timestamp       created_at;
    Timestamp       scheduled_at;
    Timestamp       started_at;
    Timestamp       finished_at;

    // --- 元数据 ---
    std::string     created_by;   // 操作人
    std::string     workflow_id;  // 所属工作流
    int             version;      // 任务定义版本号
};
```

### 3.2 任务 FSM 状态机

```
                    ┌─────────────────────────────────────────────┐
                    │                                             │
        submit()    │  dependencies_met()   execute_done()       │
  ──────────►  PENDING ──────────► READY ──────────► RUNNING    │
                    │                │        │          │        │
             cancel │         cancel │        │ error    │ done   │
                    ▼                ▼        ▼          ▼        │
                CANCELLED        CANCELLED  FAILED   COMPLETED   │
                                              │          │        │
                                    retry()   │          │        │
                                              └──► PENDING        │
                                                                  │
                    pause()                                       │
              RUNNING ──────────────────────────► PAUSED         │
                    ▲                                │            │
                    └────────────────────────────────┘            │
                              resume()                            │
                                                                  │
                    rollback()                                     │
              FAILED  ──────────────────────────► ROLLING_BACK   │
                                                        │         │
                                                        ▼         │
                                                   ROLLED_BACK    │
                                                                  │
              RUNNING / PAUSED ──── checkpoint() ──► (snapshot)  │
                                                                  │
└─────────────────────────────────────────────────────────────────┘

合法状态：
PENDING | READY | RUNNING | PAUSED | COMPLETED | FAILED |
CANCELLED | ROLLING_BACK | ROLLED_BACK | WAITING_FOR_HUMAN
```

```cpp
enum class TaskState {
    Pending,          // 已提交，等待依赖
    Ready,            // 依赖满足，等待执行器
    Running,          // 执行中
    Paused,           // 人工暂停（保存Checkpoint）
    Completed,        // 成功完成
    Failed,           // 失败（等待重试/人工处理）
    Cancelled,        // 已取消
    RollingBack,      // 回滚中
    RolledBack,       // 回滚完成
    WaitingForHuman,  // 超过重试次数，等待人工干预
};
```

### 3.3 任务图（TaskGraph）

```cpp
// 一个工作流对应一个 TaskGraph
class TaskGraph {
public:
    WorkflowId       id;
    std::string      name;
    int              version;
    GraphState       state;

    // 有向无环图（DAG）
    std::unordered_map<TaskId, Task>            tasks;
    std::unordered_map<TaskId, std::vector<TaskId>> edges; // 依赖边

    // 序列化为 JSON / YAML（用于持久化和版本控制）
    nlohmann::json   to_json() const;
    static TaskGraph from_json(const nlohmann::json&);

    // 拓扑排序（调度基础）
    std::vector<TaskId> topological_sort() const;

    // 关键路径分析（优化调度顺序）
    std::vector<TaskId> critical_path() const;
};
```

---

## 4. DAG 调度引擎

### 4.1 调度职责

```
调度引擎不关心"怎么做"，只关心"谁先做、谁后做、给谁做"。
```

| 职责 | 说明 |
|------|------|
| 依赖解析 | 检测哪些任务的前置任务已全部完成 |
| 优先级排序 | 多个 READY 任务时，按优先级 + 等待时长决定顺序 |
| 资源匹配 | 将任务分配到空闲且能力匹配的执行器 |
| 流量控制 | 防止任务洪峰打垮系统（令牌桶 / 漏桶） |
| 死锁检测 | 检测循环依赖或相互等待 |
| 资源隔离 | 不同优先级的任务使用不同的执行器资源池 |

### 4.2 调度器核心接口

```cpp
class Scheduler {
public:
    // 提交工作流（包含 TaskGraph）
    WorkflowId submit(TaskGraph graph, SubmitOptions opts);

    // 查询工作流状态
    WorkflowStatus query(WorkflowId id) const;

    // 人工干预接口（委托给 InterventionService）
    void pause(TaskId id, std::string reason);
    void resume(TaskId id);
    void cancel(TaskId id, CancelScope scope = CancelScope::Single);
    void retry(TaskId id, RetryScope scope = RetryScope::FromCheckPoint);
    void force_complete(TaskId id); // 强制标记完成（跳过步骤，记录审计）

    // 优先级调整（运行中）
    void reprioritize(TaskId id, Priority new_priority);

private:
    // 调度循环（驱动状态机转换）
    void dispatch_loop();

    // 依赖满足检测
    bool dependencies_met(const Task& task) const;

    // 执行器选择策略（可插拔）
    ExecutorId select_executor(const Task& task);

    // 流量控制（令牌桶）
    RateLimiter rate_limiter_;
};
```

### 4.3 调度策略（可插拔）

```cpp
// 策略接口
class SchedulingPolicy {
public:
    virtual ExecutorId assign(const Task& task,
                               const std::vector<ExecutorInfo>& available) = 0;
};

// 实现示例
class RoundRobinPolicy    : public SchedulingPolicy { ... };
class DeviceAffinityPolicy: public SchedulingPolicy { ... }; // 优先同设备
class LoadBalancePolicy   : public SchedulingPolicy { ... };
class PriorityFirstPolicy : public SchedulingPolicy { ... };
```

---

## 5. 执行器层

### 5.1 执行器类型

```
执行器 (Executor) = "任务的运行容器"，不包含业务逻辑。
业务逻辑由 Task 自身定义（或由插件提供）。
```

| 执行器类型 | 适用场景 |
|------------|---------|
| `ThreadPoolExecutor` | 通用计算任务、数据处理 |
| `CoroutineExecutor` | I/O 密集型、设备等待轮询 |
| `DeviceExecutor` | 直接绑定到物理设备，串行操作 |
| `ScriptExecutor` | 执行用户脚本任务（沙箱隔离） |

### 5.2 执行器接口

```cpp
class IExecutor {
public:
    virtual ~IExecutor() = default;

    // 生命周期
    virtual void    start() = 0;
    virtual void    shutdown(ShutdownMode mode) = 0; // 优雅/强制

    // 任务执行
    virtual Future<TaskResult> submit(Task task) = 0;
    virtual void    cancel(TaskId id) = 0;
    virtual void    pause(TaskId id) = 0;
    virtual void    resume(TaskId id) = 0;

    // 状态查询
    virtual ExecutorStatus  status() const = 0;
    virtual int             queue_depth() const = 0;
    virtual float           load_factor() const = 0; // 0.0~1.0
};
```

### 5.3 子任务处理

```cpp
// 复合任务（Composite Task）内部会产生子任务
// 子任务共享父任务的 workflow_id，但有独立的 task_id
struct SubTask {
    TaskId          id;
    TaskId          parent_id;
    int             sequence; // 子任务序号
    TaskState       state;
    // ... 其他字段同 Task
};

// 父任务 COMPLETED 的条件：
//   所有子任务都 COMPLETED
// 父任务 FAILED 的触发：
//   任意关键子任务 FAILED 且重试耗尽
```

---

## 6. 容错、干预与恢复机制

### 6.1 重试策略（RetryPolicy）

```cpp
struct RetryPolicy {
    int     max_attempts    = 3;
    bool    auto_retry      = true;    // false → 失败后等待人工干预

    // 退避策略
    enum class Backoff { Fixed, Linear, Exponential, Jitter };
    Backoff backoff = Backoff::Exponential;
    std::chrono::milliseconds base_delay{500};
    std::chrono::milliseconds max_delay{30'000};

    // 错误分类（决定是否可重试）
    enum class ErrorClass {
        Transient,   // 可重试：设备暂时不可用、网络抖动
        Permanent,   // 不可重试：参数错误、权限错误
        Unknown,     // 默认重试，记录日志
    };
    std::function<ErrorClass(const TaskError&)> classifier;

    // 重试范围
    enum class RetryScope {
        FromCheckPoint,  // 从上次检查点恢复（推荐）
        FromBeginning,   // 整个任务重新开始
    };
    RetryScope scope = RetryScope::FromCheckPoint;
};
```

### 6.2 断路器（Circuit Breaker）

```
对设备级别实现断路器，防止反复调用故障设备浪费资源。

状态机：
  CLOSED（正常）──[连续失败 N 次]──► OPEN（熔断）
      ▲                                   │
      │            [等待冷却期]            ▼
      └──────────────────────── HALF_OPEN（探测）
                     [探测成功]
```

```cpp
class CircuitBreaker {
    enum class State { Closed, Open, HalfOpen };
    
    int     failure_threshold  = 5;       // 连续失败次数触发熔断
    int     success_threshold  = 2;       // 半开状态成功几次后恢复
    Duration cooldown_period   = 30s;     // 熔断后冷却时间

public:
    bool can_execute() const;             // 是否允许调用
    void on_success();
    void on_failure(const TaskError&);
};
```

### 6.3 Checkpoint 机制

```cpp
// Checkpoint 是任务执行进度的快照
struct CheckPoint {
    TaskId          task_id;
    int             stage_index;    // 当前执行到第几个子阶段
    std::string     stage_name;     // 阶段名（便于人工查看）
    nlohmann::json  stage_output;   // 该阶段的输出数据（序列化）
    nlohmann::json  context;        // 任务上下文（全量，可选择性快照）
    Timestamp       saved_at;
    std::string     saved_by;       // "system" 或操作人

    // 持久化到 DB（任务暂停/失败时自动保存）
    void save(IDatabase& db);
    static CheckPoint load(IDatabase& db, TaskId id);
};
```

**何时保存 Checkpoint：**

| 触发时机 | 说明 |
|----------|------|
| 每个子阶段完成时 | 最细粒度，重试代价最小 |
| 人工暂停时 | 保存当前上下文 |
| 定时快照（可配置间隔） | 防止长任务中途崩溃 |
| 重要业务节点（显式标注） | 业务层主动调用 |

### 6.4 人工干预操作矩阵

| 操作 | 允许的状态 | 效果 | 是否记录审计 |
|------|-----------|------|------------|
| 暂停 (Pause) | Running | → Paused + 保存Checkpoint | ✅ |
| 恢复 (Resume) | Paused | → Running（从Checkpoint） | ✅ |
| 取消（单个） | Pending/Ready/Paused | → Cancelled | ✅ |
| 取消（级联） | 任何非终态 | 取消所有依赖后置任务 | ✅ |
| 重试（从检查点） | Failed | → Pending（从Checkpoint） | ✅ |
| 重试（从头） | Failed/RolledBack | → Pending（清空Checkpoint） | ✅ |
| 强制完成 | Running/Paused | → Completed（跳过，需要二次确认） | ✅ + ⚠️警告 |
| 强制失败 | Running | → Failed（紧急停止设备） | ✅ + 理由必填 |
| 调整优先级 | Pending/Ready | 重新排队 | ✅ |
| 回滚 | Failed | → RollingBack → RolledBack | ✅ |

### 6.5 降级策略

```cpp
// 可为每个任务节点配置降级方案
struct FallbackPolicy {
    enum class FallbackType {
        Skip,             // 跳过本步骤，继续后续流程
        UseDefaultValue,  // 用默认值代替本步骤输出
        AlternativeTask,  // 用备用任务代替
        HumanIntervention,// 通知人工处理
        AbortWorkflow,    // 终止整个工作流
    };

    FallbackType  type;
    TaskId        alternative_task_id; // 仅 AlternativeTask 时有效
    nlohmann::json default_output;    // 仅 UseDefaultValue 时有效
    Duration      human_wait_timeout; // 等待人工响应的超时时间
};
```

---

## 7. 审计日志与持久化

### 7.1 审计事件类型

```cpp
enum class AuditEventType {
    // 任务生命周期
    TaskSubmitted, TaskStarted, TaskPaused, TaskResumed,
    TaskCompleted, TaskFailed, TaskCancelled, TaskRetried,
    TaskRolledBack, CheckPointSaved,

    // 人工操作
    HumanPaused, HumanResumed, HumanCancelled, HumanForcedComplete,
    HumanForcedFailed, HumanRetriedFromCheckPoint,

    // 系统事件
    DeviceConnected, DeviceDisconnected, DeviceError,
    CircuitBreakerOpened, CircuitBreakerClosed,
    WatchdogTriggered, SystemShutdown,

    // 工作流级别
    WorkflowCreated, WorkflowStarted, WorkflowCompleted,
    WorkflowCancelled, WorkflowRolledBack,
};

struct AuditEvent {
    EventId         id;
    AuditEventType  type;
    TaskId          task_id;      // 可为空（工作流级别事件）
    WorkflowId      workflow_id;
    std::string     actor;        // "system" / 用户名
    nlohmann::json  before_state; // 状态变化前快照
    nlohmann::json  after_state;  // 状态变化后快照
    std::string     reason;       // 操作原因（人工操作时必填）
    Timestamp       occurred_at;
    std::string     machine_id;   // 哪台机器/服务器
};
```

### 7.2 持久化策略

```
本地 SQLite（主）
    │── 任务状态表 (tasks)
    │── 检查点表 (checkpoints)
    │── 审计日志表 (audit_events)
    │── 工作流定义表 (workflow_definitions)
    └── 设备状态表 (device_states)

RocksDB（可选，高频写日志）
    └── 实时日志流（key: timestamp, value: log entry）

云端备份（可选）
    └── 增量同步 → S3 / 自建对象存储
        - 触发时机：任务完成/失败、每日定时、手动触发
        - 加密传输（AES-256）
```

### 7.3 数据库表设计（核心表）

```sql
-- 工作流定义（版本控制）
CREATE TABLE workflow_definitions (
    id          TEXT PRIMARY KEY,
    name        TEXT NOT NULL,
    version     INTEGER NOT NULL,
    graph_json  TEXT NOT NULL,   -- 序列化的 TaskGraph
    created_by  TEXT,
    created_at  DATETIME,
    is_active   BOOLEAN DEFAULT TRUE
);

-- 任务实例（运行时状态）
CREATE TABLE tasks (
    id              TEXT PRIMARY KEY,
    workflow_id     TEXT NOT NULL,
    parent_task_id  TEXT,
    name            TEXT,
    kind            TEXT,
    state           TEXT NOT NULL,
    priority        INTEGER,
    params_json     TEXT,
    target_device   TEXT,
    created_at      DATETIME,
    started_at      DATETIME,
    finished_at     DATETIME,
    retry_count     INTEGER DEFAULT 0,
    error_message   TEXT
);

-- 检查点
CREATE TABLE checkpoints (
    id              TEXT PRIMARY KEY,
    task_id         TEXT NOT NULL,
    stage_index     INTEGER,
    stage_name      TEXT,
    stage_output    TEXT,          -- JSON
    context_json    TEXT,          -- JSON
    saved_at        DATETIME,
    saved_by        TEXT
);

-- 审计日志（只追加，禁止修改/删除）
CREATE TABLE audit_events (
    id              TEXT PRIMARY KEY,
    type            TEXT NOT NULL,
    task_id         TEXT,
    workflow_id     TEXT,
    actor           TEXT,
    before_state    TEXT,          -- JSON
    after_state     TEXT,          -- JSON
    reason          TEXT,
    occurred_at     DATETIME NOT NULL
) STRICT;
```

---

## 8. 设备抽象与通信协议

### 8.1 设备抽象层（HAL - Hardware Abstraction Layer）

```cpp
// 所有物理设备必须实现此接口
class IDevice {
public:
    virtual ~IDevice() = default;

    // 生命周期
    virtual bool   connect(const DeviceConfig& cfg) = 0;
    virtual void   disconnect() = 0;
    virtual bool   is_connected() const = 0;

    // 能力查询（用于调度器匹配）
    virtual std::vector<TaskKind> supported_task_kinds() const = 0;

    // 执行
    virtual Future<DeviceResult> execute(const DeviceCommand& cmd) = 0;
    virtual void                 abort() = 0;          // 紧急停止

    // 状态
    virtual DeviceStatus         status() const = 0;
    virtual DeviceHealthInfo     health() const = 0;   // 心跳数据

    // 事件订阅（设备主动上报）
    virtual void on_event(std::function<void(DeviceEvent)> cb) = 0;
};
```

### 8.2 通信协议适配器

```cpp
// 协议适配器接口（由插件系统动态加载）
class IProtocolAdapter {
public:
    virtual std::string     protocol_name() const = 0;
    virtual bool            connect(const std::string& endpoint) = 0;
    virtual ByteBuffer      send_receive(const ByteBuffer& request) = 0;
    virtual void            close() = 0;
};

// 内置适配器
class RS232Adapter    : public IProtocolAdapter { ... };
class RS485Adapter    : public IProtocolAdapter { ... };
class ModbusAdapter   : public IProtocolAdapter { ... };
class OPCUAAdapter    : public IProtocolAdapter { ... };
class MQTTAdapter     : public IProtocolAdapter { ... };
class HTTPAdapter     : public IProtocolAdapter { ... };

// 用户可通过插件添加自定义协议
// class MyCustomAdapter : public IProtocolAdapter { ... };
```

### 8.3 设备报文规范（可配置）

```yaml
# device_protocol.yaml（用户可编辑）
device:
  id: "centrifuge_001"
  name: "离心机-01号"
  protocol: "modbus_rtu"
  endpoint: "COM3"
  baud_rate: 9600

commands:
  - name: "start"
    request_template: "01 06 00 01 {speed:uint16_be} {duration:uint16_be}"
    response_parser:
      success_pattern: "01 06 00 01"
      timeout_ms: 5000

  - name: "query_status"
    request_template: "01 03 00 00 00 02"
    response_parser:
      fields:
        - {name: "state",    offset: 3, type: "uint8"}
        - {name: "rpm",      offset: 4, type: "uint16_be"}
        - {name: "temp_c",   offset: 6, type: "float32_be"}
```

---

## 9. 插件系统

### 9.1 架构设计

```
插件加载器 (PluginLoader)
    │── 扫描 ./plugins/ 目录下的 .so / .dll
    │── 调用插件入口 `plugin_init()`
    │── 注册插件提供的组件到对应的 Registry
    │
    ▼
Registry（注册表）
    ├── DeviceRegistry      → 新设备类型
    ├── ProtocolRegistry    → 新通信协议
    ├── TaskRegistry        → 新任务类型（含自定义 FSM）
    └── SchedulingRegistry  → 新调度策略
```

### 9.2 插件接口

```cpp
// 每个插件必须导出此结构体
struct PluginDescriptor {
    const char* name;
    const char* version;
    const char* author;

    // 初始化（注册到 Registry）
    void (*init)(PluginContext& ctx);

    // 清理
    void (*shutdown)();
};

// 插件入口（extern "C" 保证 ABI 兼容）
extern "C" PluginDescriptor* plugin_init();
```

### 9.3 用户动态添加设备流程

```
1. 用户在 UI 填写设备信息（名称、协议、地址、报文规范）
2. 系统将配置写入 device_protocol.yaml
3. 若协议是内置的（RS232/Modbus/...）→ 直接实例化内置适配器
4. 若协议是自定义的 → 加载用户提供的插件 .so
5. 设备注册到 DeviceRegistry，调度器可立即感知到新设备
6. 在任务图编辑器中，新设备出现在可选设备列表中
```

---

## 10. 事件总线与内部通信

### 10.1 为什么需要事件总线

各模块（调度器、执行器、设备层、UI）之间的通信如果直接依赖，会造成紧耦合。事件总线（Event Bus）是解耦的核心手段。

```
调度器           执行器
  │ publish(TaskStarted)  │
  └─────────────►  EventBus  ◄─── 设备层 publish(DeviceError)
                     │
                     ├───► 审计引擎（订阅所有事件）
                     ├───► UI 层（订阅状态变化事件）
                     ├───► 看门狗（订阅 Running 事件，计时）
                     └───► 容错引擎（订阅 Failed 事件，决策重试）
```

```cpp
class EventBus {
public:
    // 发布
    void publish(const Event& event);

    // 订阅（返回订阅句柄，RAII 管理生命周期）
    [[nodiscard]] Subscription subscribe(EventType type,
                                          std::function<void(Event)> handler);

    // 支持过滤（只订阅特定 workflow / task）
    [[nodiscard]] Subscription subscribe_filtered(
        EventType type,
        EventFilter filter,
        std::function<void(Event)> handler);
};
```

---

## 11. 健康监控与看门狗

### 11.1 任务级看门狗（Thread Watchdog）

```cpp
class TaskWatchdog {
    struct WatchEntry {
        TaskId      task_id;
        Timestamp   last_heartbeat;
        Duration    timeout;
        int         warning_count;
    };

public:
    // 任务开始时注册
    void watch(TaskId id, Duration timeout);

    // 任务执行时定期调用（心跳）
    void heartbeat(TaskId id);

    // 任务完成时注销
    void unwatch(TaskId id);

private:
    // 后台线程定期扫描超时条目
    void watchdog_loop();

    // 超时处理：发送告警 → 尝试取消 → 强制终止
    void on_timeout(const WatchEntry& entry);
};
```

### 11.2 系统级看门狗

```cpp
// 独立进程（或独立线程 + 定时器）监控主进程健康状态
// 主进程定期向看门狗更新心跳（写文件 / 共享内存 / 命名管道）
// 若超过 timeout 未收到心跳 → 触发主进程重启
// 重启后：从 DB 读取所有 Running 状态任务 → 恢复到 Paused 状态等待人工确认

class SystemWatchdog {
public:
    // 主进程调用
    void kick();                    // 更新心跳

    // 看门狗进程调用
    void monitor_loop();            // 监控循环
    void on_main_process_died();    // 触发恢复流程
};
```

### 11.3 可观测性指标

```
系统应暴露以下指标（Prometheus格式，或写入本地监控数据库）：

任务指标：
  task_total{state="running|completed|failed|..."}
  task_duration_seconds{percentile="p50|p95|p99"}
  task_retry_total{reason="transient|permanent"}
  task_checkpoint_save_duration_seconds

执行器指标：
  executor_queue_depth{executor_id="..."}
  executor_load_factor{executor_id="..."}
  executor_task_throughput_per_second

设备指标：
  device_error_total{device_id="...", error_type="..."}
  device_circuit_breaker_state{device_id="..."}  # 0=closed, 1=open
  device_response_latency_ms{device_id="..."}

系统指标：
  system_memory_usage_bytes
  watchdog_heartbeat_age_seconds
```

---

## 12. 配置与版本管理

### 12.1 配置分层

```
配置优先级（高→低覆盖）：

1. 运行时动态配置（API / UI 修改，热加载）
2. 环境变量
3. 本地配置文件 config.yaml
4. 默认值（代码内置）
```

### 12.2 任务图版本控制

```
每次保存 WorkflowDefinition，version 字段 +1。
历史版本保留在 workflow_definitions 表中（is_active 标记当前版本）。

支持：
- 查看历史版本差异
- 回滚到历史版本
- 对比两个版本的执行结果差异（审计辅助）
```

---

## 13. 安全与权限

> 实验室场景下，人工干预操作（强制完成、回滚等）需要权限控制，防止误操作。

### 13.1 操作权限矩阵

| 角色 | 提交任务 | 暂停/恢复 | 取消 | 强制完成 | 强制失败 | 查看审计 | 修改配置 |
|------|---------|----------|------|---------|---------|---------|---------|
| Operator（操作员） | ✅ | ✅ | ✅ | ❌ | ❌ | ✅ | ❌ |
| Engineer（工程师） | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Auditor（审计员） | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ | ❌ |
| Admin（管理员） | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |

### 13.2 高危操作二次确认

```
强制完成、强制失败、取消整个工作流 → 必须输入操作原因 → 发送到审计日志
可选：需要第二个 Engineer 级别用户确认（双人操作规则）
```

---

## 14. 开源库选型

### 14.1 核心库

| 用途 | 推荐库 | 理由 |
|------|--------|------|
| **DAG 任务调度** | [Taskflow](https://github.com/taskflow/taskflow) | Header-only C++17，支持 DAG、条件分支、异步任务，文档极好 |
| **状态机** | [Boost.SML](https://boost-ext.github.io/sml/) | 零开销状态机，编译期验证转换合法性，性能优异 |
| **线程池** | [BS::thread_pool](https://github.com/bshoshany/thread-pool) | 轻量 Header-only，比手写线程池稳定 |
| **JSON 序列化** | [nlohmann/json](https://github.com/nlohmann/json) | 使用最广泛，API 优雅 |
| **YAML 配置** | [yaml-cpp](https://github.com/jbeder/yaml-cpp) | 设备配置文件解析 |
| **本地数据库** | [SQLite](https://sqlite.org/) + [SQLiteCpp](https://github.com/SRombauts/SQLiteCpp) | 零部署，ACID，C++ 封装优雅 |
| **日志** | [spdlog](https://github.com/gabime/spdlog) | 极高性能，异步日志，格式化丰富 |
| **事件总线** | [eventpp](https://github.com/wqking/eventpp) | 线程安全事件分发，支持过滤器 |

### 14.2 通信协议库

| 协议 | 推荐库 |
|------|--------|
| Modbus RTU/TCP | [libmodbus](https://github.com/stephane/libmodbus) |
| OPC-UA | [open62541](https://github.com/open62541/open62541)（最活跃的开源 OPC-UA 库）|
| MQTT | [Eclipse Paho MQTT C++](https://github.com/eclipse/paho.mqtt.cpp) |
| 串口（RS232/485） | [serial](https://github.com/wjwwood/serial) 或直接用 termios/Win32 API |
| 通用网络 | [Asio](https://think-async.com/Asio/)（standalone，无需 Boost）|

### 14.3 其他工具库

| 用途 | 推荐库 |
|------|--------|
| UUID 生成 | [stduuid](https://github.com/mariusbancila/stduuid) |
| 时间处理 | C++20 `<chrono>` / [date](https://github.com/HowardHinnant/date) |
| 配置热加载 | [efsw](https://github.com/SpartanJ/efsw)（文件系统监控） |
| 插件加载 | `dlopen` / `LoadLibrary` 封装，或 [Poco::ClassLoader](https://pocoproject.org/) |
| 单元测试 | [Catch2](https://github.com/catchorg/Catch2) |
| 性能测试 | [Google Benchmark](https://github.com/google/benchmark) |
| 指标暴露 | [prometheus-cpp](https://github.com/jupp0r/prometheus-cpp) |

---

## 15. 补充项：你的盲区

以下是你的原始设计中**未覆盖**的关键点：

### 15.1 事件溯源（Event Sourcing）⚠️ 重要

```
不要只存储"当前状态"，要存储"导致状态变化的事件序列"。

好处：
- 任意时间点的状态都可以通过重放事件序列还原
- 审计更强（不仅知道"最终状态"，还知道"如何到达的"）
- 便于调试：重现 bug 只需重放事件

实现：audit_events 表本质上就是事件流，
      读取任意 task_id 的事件序列即可重建任务完整历史。
```

### 15.2 任务图版本控制 ⚠️ 中等重要

```
生产环境中，工作流定义会迭代升级。
正在执行中的任务实例应该绑定到创建时的工作流版本，
不受后续版本升级影响（否则可能产生不一致行为）。
```

### 15.3 死锁检测 ⚠️ 中等重要

```
DAG 提交时应检测循环依赖：
  A → B → C → A（循环）→ 拒绝提交，给出错误信息

运行时检测相互等待（资源死锁）：
  使用等待图（Wait-For Graph）检测环
```

### 15.4 资源配额管理 ⚠️ 中等重要

```
多个工作流并发时，需要防止某个工作流占用全部设备资源：

每个工作流可配置：
- max_concurrent_tasks: 最多并行执行几个任务
- device_quotas: 最多占用某类设备几台
- cpu_quota / memory_quota: 计算资源限制
```

### 15.5 任务超时管理 ⚠️ 重要

```cpp
struct TimeoutPolicy {
    Duration  soft_timeout;   // 超时后发告警，继续执行
    Duration  hard_timeout;   // 超时后强制取消任务
    bool      save_checkpoint_on_soft_timeout = true;
};
```

### 15.6 幂等性保证 ⚠️ 重要

```
对于设备操作，必须考虑：
  如果指令已经发送，但响应丢失 → 重试时设备是否会执行两次？

解决方案：
  1. 设备端幂等：每个指令携带唯一 command_id，设备去重
  2. 应用端幂等：发送前查询设备状态，确认未执行才发送
  3. 操作设计为幂等：启动命令改为"确保处于运行中"而非"开始运行"
```

### 15.7 测试策略

```
单元测试：
  - FSM 状态转换覆盖率 100%
  - RetryPolicy 所有分支路径
  - DAG 拓扑排序正确性

集成测试：
  - 使用 Mock 设备（软件模拟器）运行完整工作流
  - 注入错误（网络中断、设备超时）验证容错逻辑

压力测试：
  - 1000个并发任务的调度稳定性
  - 审计日志写入不影响调度延迟
```

---

## 附录：模块依赖关系图

```
UI Layer
    │
Application Layer (WorkflowManager, InterventionService, AuditService)
    │
    ├── Scheduler (DAG Engine, Priority Queue, Rate Limiter)
    │       └── depends on: TaskGraph, FSM, SchedulingPolicy
    │
    ├── ExecutorPool (ThreadPool, CoroutinePool, DeviceExecutor)
    │       └── depends on: Task, CheckPoint, WatchDog
    │
    ├── AuditEngine (EventSourcing, DB Writer, Cloud Sync)
    │       └── depends on: AuditEvent, SQLite, EventBus
    │
    └── EventBus (pub/sub, filtering)

Infrastructure Layer
    ├── DeviceRegistry + HAL (IDevice, IProtocolAdapter)
    ├── PluginLoader (dlopen, PluginDescriptor)
    ├── Database (SQLite, RocksDB)
    ├── WatchDog (TaskWatchdog, SystemWatchdog)
    └── ConfigManager (YAML, hot-reload)
```

---

*设计书版本：v1.0 | 编写日期：2026-05-06 | 适用场景：实验室/工业设备自动化*
