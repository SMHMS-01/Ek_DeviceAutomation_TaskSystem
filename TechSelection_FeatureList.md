# 设备自动化任务系统 — 技术选型参考与功能清单 v1.0

> **文档定位**：本文档是 v1.1 设计书的配套实施参考，解决"用什么库、做什么功能、按什么顺序、为什么"四个问题。
> 所有选型决策追溯至"控制论闭环所需七条先验约束"，保证选型有原则依据而非凭经验拍板。

---

## 目录

1. [设计原则：先验/经验框架的工程映射](#1-设计原则)
2. [功能清单（P0 / P1 / P2）](#2-功能清单)
3. [技术选型表（分层）](#3-技术选型表)
4. [模块构建依赖顺序](#4-模块构建依赖顺序)
5. [v1.1 设计书补丁建议](#5-v11-设计书补丁建议)
6. [已知简化项（Demo vs 生产）](#6-已知简化项)

---

## 1. 设计原则

### 1.1 七条先验约束 → 模块 → 库的映射

| 先验约束 | 对应模块 | P0 实现 | P1/P2 升级 |
|---------|---------|---------|-----------|
| **① 同一性** Identity | `TaskId` / `WorkflowId` | `stduuid`（header-only UUID）| — |
| **② 状态单值性** State Monovalence | `TaskStateMachine` | 手写 FSM | Boost.SML（状态数 > 10 时）|
| **③ 因果序** Causal Ordering | `TaskGraph` | 手写 DAG + 拓扑排序 | Taskflow（执行层集成）|
| **④ 终止性** Termination | `TaskWatchdog` + `TimeoutPolicy` | 手写计时器 | — |
| **⑤ 错误一阶公民** Error First-Class | `TaskStateMachine::Failed` 状态 + `RetryPolicy` | 手写 | — |
| **⑥ 可观测性** Observability | `AuditService` + `SqliteDatabase` | SQLite3 C API + SQLiteCpp | prometheus-cpp（P2）|
| **⑦ 取消可达性** Cancellation | `ITaskHandle::cancel()` + `CancelToken` | 手写原子标志 | C++20 `stop_token` |

**原则**：先验约束对应的模块，用最稳定、最少依赖的库实现。经验层（设备通信、插件）用成熟的领域专用库。

### 1.2 两层架构的库归属

```
控制平面（先验层）         ← stduuid / 手写FSM / SQLite / spdlog / EventBus
        │
    ITaskHandle（窄接口）  ← 纯虚类，零依赖
        │
执行平面（经验层）         ← Taskflow / BS::thread_pool / libmodbus /
                              open62541 / yaml-cpp / 插件加载器
```

---

## 2. 功能清单

### P0 — 可编译、可测试、可审计的最小骨架

> **当前状态（v1.1）**：已完成。对应 `PHASE 2.1` 验收通过。

| 功能 | 模块 | 验收标准 |
|------|------|---------|
| 任务身份（UUID） | `TaskId` | 生成的ID在进程生命周期内唯一，可序列化为字符串 |
| 任务FSM（7状态） | `TaskStateMachine` | 所有合法转换通过，所有非法转换抛异常 |
| DAG构建与校验 | `TaskGraph` | 有向无环图提交通过，循环图在`add_dependency`或提交时拒绝 |
| 拓扑排序与READY判定 | `TaskGraph` | 依赖全完成的任务正确进入READY集合 |
| 同步调度器 | `SimpleScheduler` | 线性/并行工作流端到端执行，审计条目数量正确 |
| 每次状态转换落库 | `AuditService` + `SqliteDatabase` | `audit_events`表记录数等于状态转换次数 |
| Schema Migration | `MigrationRunner` | 幂等应用，失败回滚，版本号单调递增 |
| 线程安全事件总线 | `EventBus` | 多线程并发发布不崩溃，订阅回调异常不传播 |
| 应用服务入口 | `WorkflowManager` | 提交工作流后返回`WorkflowId`，可查询状态 |
| 基础日志（可选spdlog）| `Logger` | spdlog存在时使用，缺失时fallback到stderr |

---

### P1 — 生产可用核心

> **目标**：真实设备可接入，多线程调度稳定，人工干预可操作。
> 对应 `PHASE 3 Scheduler & Executors` + `PHASE 4 Device & Intervention`。

| 功能 | 模块 | 依赖库 | 验收标准 |
|------|------|--------|---------|
| 线程池执行器 | `ThreadPoolExecutor` | `BS::thread_pool` | 1000个任务并发执行无死锁，graceful shutdown |
| 设备串行执行器 | `DeviceExecutor` | `IDevice`接口 | 同一设备的任务串行执行，不并发 |
| 执行器取消协议 | `CancelToken` | C++20 `stop_token` / 手写 | cancel()调用后任务最终进入Cancelled状态 |
| 任务超时（软/硬）| `TimeoutPolicy` | 手写计时器 | 软超时保存Checkpoint并告警；硬超时强制取消 |
| 任务级看门狗 | `TaskWatchdog` | 手写后台线程 | 超时未心跳的Running任务触发告警并处理 |
| Checkpoint保存/恢复 | `CheckPoint` + `SqliteDatabase` | SQLiteCpp | 从Checkpoint恢复后任务从正确阶段重新执行 |
| 重启恢复流程 | `AuditService` | SQLite查询 | 重启后Running任务恢复为Paused，等待人工确认 |
| **设备状态协调层** ⭐ | `DeviceStateReconciler` | `IDevice` | 见下方专项说明 |
| Mock设备（可注入失败）| `MockDevice` | 无 | 支持配置失败概率、延迟、错误类型 |
| Modbus设备适配 | `ModbusDevice` | `libmodbus` | 连接/断开/读写线圈和寄存器 |
| RS232/RS485设备适配 | `SerialDevice` | `serial`库 | 打开端口、配置波特率、读写字节 |
| 设备配置文件解析 | `DeviceConfigLoader` | `yaml-cpp` | 解析`device_protocol.yaml`，构建`DeviceConfig` |
| 人工干预服务 | `InterventionService` | 无 | pause/resume/cancel/retry/force_complete，原因必填，记审计 |
| 断路器（设备级）| `CircuitBreaker` | 手写 | 连续失败N次熔断，冷却后半开探测 |
| 基础RetryPolicy | `RetryPolicy` | 手写 | Fixed/Exponential退避，Transient/Permanent分类 |
| 规范消息格式 ⭐ | `CanonicalMessage` | `nlohmann/json` | 设备间数据流通过规范格式解耦，无点对点依赖 |

#### ⭐ 设备状态协调层（DeviceStateReconciler）— v1.1 缺失项

这是从"FSM状态 ≠ 物理设备状态"这一生产差距中推导出的必要模块。

```
问题：
  软件FSM认为 Task = Running
  软件崩溃后重启
  设备可能：已完成 / 还在运行 / 已失败 / 状态未知

职责（ONLY）：
  - 重启时查询目标设备的实际状态
  - 将设备实际状态与FSM期望状态进行比对
  - 输出协调结论：Consistent / DeviceAhead / DeviceBehind / Unknown

不做的事：
  - 不决定如何处理不一致（这是InterventionService或RetryPolicy的职责）
  - 不直接修改FSM状态

接口：
  ReconcileResult reconcile(TaskId, IDevice*, TaskState fsm_state);

输出结论的处理策略（经验层，可配置）：
  Consistent   → 继续执行
  DeviceAhead  → 更新FSM为实际完成状态（需审计标注"协调更新"）
  DeviceBehind → 重新发送指令（需幂等Token）
  Unknown      → 转WaitingForHuman
```

---

### P2 — 扩展能力

> **目标**：插件生态、可观测性、云端备份、OPC-UA等高级设备协议。
> 非MVP强依赖，按需实现。

| 功能 | 模块 | 依赖库 | 备注 |
|------|------|--------|------|
| In-Process插件加载 | `PluginLoader` | `dlopen`/`LoadLibrary` 封装 | 见插件系统选型章节 |
| Out-of-Process插件 | `PluginProxy` | `Asio`（standalone）+ IPC | 崩溃隔离；复杂度+1量级 |
| 动态设备注册（UI）| `DeviceRegistry` + UI配置 | `yaml-cpp` | 用户填写 → 写配置 → 热加载 |
| OPC-UA设备适配 | `OpcUaDevice` | `open62541` | 工业标准，医疗设备常用 |
| MQTT消息总线适配 | `MqttAdapter` | `Eclipse Paho MQTT C++` | IoT设备接入 |
| 指标暴露 | `MetricsCollector` | `prometheus-cpp` | 暴露任务/设备/调度器指标 |
| 配置热加载 | `ConfigWatcher` | `efsw` | 文件系统监控，设备配置变更自动重载 |
| 云端审计备份 | `CloudAuditSync` | `libcurl` 或 SDK | 增量同步，加密传输 |
| 任务图可视化编辑 | UI层 | 不在C++核心内 | DAG JSON导出，前端渲染 |

---

## 3. 技术选型表

### 3.1 控制平面库

| 用途 | 选型 | 版本/获取 | 选型理由 | 迁移触发条件 |
|------|------|----------|---------|------------|
| **UUID生成** | `stduuid` | header-only, GitHub | 零依赖，符合RFC 4122，跨平台 | — |
| **JSON序列化** | `nlohmann/json` | header-only v3.x | API直觉，广泛使用，Checkpoint序列化 | 性能热路径出现时评估simdjson |
| **SQLite封装** | `SQLiteCpp` | v3.x | RAII包装sqlite3，事务/查询清晰 | 当前P0用sqlite3 C API；查询复杂度上升后迁移 |
| **日志** | `spdlog` | v1.x，可选 | 异步日志，格式化丰富，缺失时fallback | — |
| **状态机** | 手写FSM（P0）→ `Boost.SML`（P1+）| header-only | SML零开销，编译期合法转换校验 | 状态数超过10或转换条件复杂时迁移 |
| **事件总线** | 手写同步（P0）→ `eventpp`（P1）| header-only | 线程安全，支持过滤器，异步分发 | 高吞吐（>10k events/s）或需要过滤时迁移 |
| **DAG调度** | 手写（P0）→ `Taskflow`（P1）| header-only C++17 | DAG + 条件分支 + 异步，文档极好 | ExecutorPool阶段评估集成 |
| **线程池** | `BS::thread_pool` | header-only | 轻量稳定，比手写线程池可靠 | ExecutorPool阶段引入 |

### 3.2 执行平面库（设备通信）

| 协议 | 选型 | 选型理由 | 适用设备举例 |
|------|------|---------|------------|
| **Modbus RTU/TCP** | `libmodbus` | 最成熟的开源Modbus库，C API，稳定 | PLC、离心机、温控仪 |
| **RS232/RS485** | `serial`（wjwwood）| 跨平台，POSIX+Win32封装 | 各类实验室串口设备 |
| **OPC-UA** | `open62541` | 最活跃的OPC-UA开源实现，C99，嵌入友好 | 工业自动化设备，医疗仪器 |
| **MQTT** | `Eclipse Paho MQTT C++` | Eclipse维护，成熟，支持QoS 0/1/2 | IoT传感器，远端设备 |
| **通用网络I/O** | `Asio`（standalone，非Boost）| 跨平台异步I/O，协程支持 | 自定义TCP/UDP协议设备 |
| **HTTP REST** | `cpp-httplib` | header-only，无重依赖 | 支持REST API的现代仪器 |

### 3.3 插件系统

| 组件 | 方案 | 实现方式 | 备注 |
|------|------|---------|------|
| **插件加载（In-Process）** | 平台原生 | Linux: `dlopen`/`dlsym`/`dlclose`；Windows: `LoadLibrary`/`GetProcAddress`/`FreeLibrary` | 自行封装为`PluginLoader`类，约200行 |
| **ABI稳定性** | C入口 + 纯虚接口 | `extern "C" PluginDescriptor* plugin_get_descriptor()` + `IDevice`纯虚类 | 不跨ABI边界传递STL容器和异常 |
| **插件发现** | 目录扫描 + 清单文件 | `std::filesystem::directory_iterator` + `plugin_manifest.yaml` | 清单文件声明插件元数据和兼容版本 |
| **版本兼容** | Major版本匹配 | `api_version.major`必须等于宿主期望值 | Minor版本向后兼容，新增接口用default实现 |
| **崩溃隔离（P2）** | Out-of-Process | 独立子进程 + `Asio`双向IPC | 插件崩溃不传染宿主，代价：接口变成序列化调用 |

### 3.4 序列化与配置

| 用途 | 选型 | 说明 |
|------|------|------|
| **任务参数/Checkpoint** | `nlohmann/json` | 运行时序列化，存入SQLite `TEXT`字段 |
| **设备配置文件** | `yaml-cpp` | `device_protocol.yaml`，用户可编辑，热加载 |
| **工作流定义文件** | `nlohmann/json` 或 `yaml-cpp` | TaskGraph的持久化格式，支持版本控制 |
| **设备间规范消息** | `nlohmann/json` + schema字符串 | `CanonicalMessage.payload_json`，schema如`"centrifuge.result.v1"` |

### 3.5 可观测性

| 用途 | 选型 | 引入时机 |
|------|------|---------|
| **结构化日志** | `spdlog` | P0已引入（可选） |
| **指标暴露** | `prometheus-cpp` | P2，需要外部监控时 |
| **分布式追踪（可选）** | OpenTelemetry C++ | P2，多进程场景 |

### 3.6 测试

| 用途 | 选型 | 说明 |
|------|------|------|
| **单元/集成测试** | `Catch2` v3 | header-only（单头文件模式），BDD风格可选，与CTest集成良好 |
| **Mock设备** | 手写`MockDevice : IDevice` | 支持注入失败概率、延迟、特定错误类型 |
| **性能基准** | `Google Benchmark` | P1引入，调度器和EventBus吞吐基准 |
| **Fuzzing（可选）** | `libFuzzer` / `AFL++` | P2，协议解析器的健壮性测试 |

---

## 4. 模块构建依赖顺序

```
第0层（无任何依赖）
  TaskId / WorkflowId / Timestamp / Priority / TaskState

第1层（仅依赖第0层）
  TaskStateMachine          ← 先验②
  CheckPoint
  RetryPolicy
  CancelToken               ← 先验⑦

第2层（依赖第0+1层）
  Task                      ← 聚合上述类型
  TaskGraph                 ← 先验③，依赖Task
  CanonicalMessage          ← 设备间数据流规范格式

第3层（依赖第0+1+2层）
  EventBus                  ← 先验⑥，零外部依赖
  Logger                    ← 依赖spdlog（可选）

第4层（依赖第0~3层）
  SqliteDatabase            ← 先验⑥持久化，依赖SQLite3/SQLiteCpp
  MigrationRunner           ← 依赖SqliteDatabase
  AuditService              ← 依赖SqliteDatabase + EventBus

第5层（依赖第0~4层）
  SimpleScheduler / Taskflow调度器   ← 先验①③④
  TaskWatchdog              ← 先验④，依赖EventBus
  DeviceStateReconciler     ← 新增，依赖IDevice + TaskStateMachine

第6层（依赖第0~5层）
  IDevice（纯虚接口）        ← 窄接口
  MockDevice                ← 依赖IDevice
  DeviceExecutor            ← 依赖IDevice + CancelToken
  ThreadPoolExecutor        ← 依赖BS::thread_pool + CancelToken
  CircuitBreaker            ← 依赖IDevice + EventBus

第7层（依赖第0~6层）
  WorkflowManager           ← 应用服务入口
  InterventionService       ← 依赖Scheduler + FSM + AuditService
  DeviceRegistry            ← 依赖IDevice工厂

第8层（依赖第0~7层）
  PluginLoader              ← 依赖DeviceRegistry + ProtocolRegistry
  DeviceConfigLoader        ← 依赖yaml-cpp + DeviceRegistry
  具体设备实现（Modbus/Serial/OpcUa）
```

---

## 5. v1.1 设计书补丁建议

以下条目是基于本文档分析，建议合并回 v1.1 的修正项：

### 补丁1：增加 DeviceStateReconciler 模块定义

在 v1.1 Section 3"模块职责边界"中增加一行：

| 模块 | 当前职责 | 不做的事 |
|------|----------|---------|
| `DeviceStateReconciler` | 重启或异常恢复时，查询物理设备实际状态，与FSM期望状态比对，输出协调结论 | 不修改FSM状态；不决定处理策略（由调用方决定） |

**原因**：v1.1 Section 5 第6条"重启时将 Running 任务恢复为 Paused"跳过了这一步——在不查询设备实际状态的前提下，直接恢复为 Paused 是一个不安全的假设。设备可能已经完成了。

### 补丁2：Section 4.1 补充设备通信库和插件系统选型

v1.1 Section 4.1 的"开源库复用策略"表缺少以下行：

| 能力 | 优先复用 | 当前处理 |
|------|----------|---------|
| **Modbus RTU/TCP** | `libmodbus` | P1 DeviceExecutor阶段引入 |
| **RS232/RS485** | `serial`（wjwwood）| P1 同上 |
| **OPC-UA** | `open62541` | P2，按需引入 |
| **YAML配置解析** | `yaml-cpp` | P1 DeviceConfigLoader引入 |
| **插件加载** | `dlopen`/`LoadLibrary`自封装 | P2，约200行封装，无需第三方库 |
| **设备间消息** | `nlohmann/json`（已有）+ schema字符串约定 | P1，CanonicalMessage结构体 |

### 补丁3：Section 2.1 补充 CanonicalMessage

在 Domain Layer 中增加：

> `CanonicalMessage`：设备间数据流的规范消息格式。包含 `schema`（如 `"centrifuge.result.v1"`）、`payload_json`、`source_device_id`、`timestamp_ms`。所有设备适配器负责私有格式 ↔ 规范格式互转，不存在设备间点对点格式依赖。

**原因**：设备间串联的可扩展性依赖于规范格式，N个设备只需N个适配器，而非N²个点对点转换器。

---

## 6. 已知简化项（Demo vs 生产）

**明确标注这些简化项的目的**：防止读者将 Demo 架构直接用于生产，同时说明架构的哪些部分是可直接迁移的。

| 简化项 | Demo 做法 | 生产实际要求 | 架构影响 |
|--------|----------|------------|---------|
| **FSM状态 ≠ 物理状态** | 假设设备响应反映真实状态 | 需要 `DeviceStateReconciler` 协调 | 中等（新增一个模块）|
| **幂等性** | 重试直接重发指令 | 需要幂等Token，先查询再发送 | 中等（接口需增加command_id）|
| **回滚的物理语义** | Task 状态改为 RolledBack | 物理样本已被处理，无法真正回滚 | 低（仅限业务理解，架构不变）|
| **插件崩溃隔离** | In-Process，插件崩溃杀死宿主 | Out-of-Process 子进程隔离 | 高（接口变为序列化调用）|
| **ABI 版本管理** | Demo 全部同一工具链编译 | 需要接口版本化和向后兼容策略 | 中等 |
| **审计日志不可篡改** | SQLite 普通写入 | 需要 append-only + 哈希链 / 外部审计数据库 | 中等（存储层替换）|
| **时钟可信度** | 系统时钟 | 医疗合规需 NTP同步 + 签名时间戳（FDA 21 CFR Part 11）| 低（替换时间戳来源）|
| **数据库规模** | SQLite 足够 | 审计日志亿级条目时需要分区/归档策略 | 低（存储层替换，不影响上层）|
| **实时性** | 任务调度延迟不确定 | 部分设备需要 < 100ms 响应 | 高（需要实时调度策略，可能需要RTOS）|

**可直接迁移到生产的部分（架构价值不打折扣）**：
- ITaskHandle 七条先验约束的接口设计
- FSM 状态合法性校验
- DAG 因果序和循环检测
- Checkpoint 存储结构
- AuditEvent 追加写入模型
- 插件接口契约（plugin_api.h）

---

## 7. 本轮贯彻记录（2026-05-16）

已落地：

- `PHASE 3.2 ExecutorPool`：先用 C++20 标准库和 `IExecutor` 窄接口完成多执行器验证，避免在接口尚未稳定时直接绑定第三方线程池。
- `PHASE 3.3 SchedulingPolicy / DeviceExecutor`：实现 `RoundRobinPolicy`、`PriorityFirstPolicy`、`DeviceAffinityPolicy` 和基于 `IDevice` 的设备串行执行器。
- `PHASE 4.1 InterventionService`：实现 pause/resume/cancel/retry/force_complete，actor/reason 必填，状态与审计落库。

保留替换点：

- `ExecutorPool` 当前基于 `std::async`，进入高压并发与 graceful shutdown 验收前优先适配 `BS::thread_pool` 或 Taskflow executor。
- `EventBus` 当前仍为同步 MVP，高吞吐设备事件进入前评估 `eventpp`。
- `SqliteDatabase` 当前仍直接使用 sqlite3 C API，复杂查询/绑定增多后评估 SQLiteCpp 或 repository 层。

新增必须跟踪项：

- `DeviceStateReconciler` 仍未实现，已记录到 `RISK_REGISTER.md` R-014。
- RateLimiter、CancelToken、TimeoutPolicy、权限矩阵和二次确认仍是 PHASE 4/5 的生产化前置项。

---

*版本：v1.0 | 日期：2026-05-16 | 配套文档：设计书 v1.1 | 当前阶段：PHASE 4.1*
