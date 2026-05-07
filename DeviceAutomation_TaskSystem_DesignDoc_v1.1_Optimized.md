# 设备自动化任务系统 — 优化设计书 v1.1

> 本版本基于 v1.0 设计书补齐工程落地缺口，并定义当前仓库的最小可验收核心。v1.1 的目标不是一次性实现全部高级能力，而是先建立可编译、可测试、可审计、可恢复扩展的主干。

## 1. v1.0 设计缺漏与修正

| 缺漏 | 风险 | v1.1 修正 |
|------|------|-----------|
| 缺少最小可交付边界 | 容易长期停留在宏观架构 | 定义 MVP：Task/FSM、TaskGraph、DAG 调度、审计、SQLite 持久化、Mock 集成验收 |
| 接口多为示意代码 | 设计难以被编译验证 | 将 Task、TaskGraph、SimpleScheduler、EventBus、SqliteDatabase 纳入可编译模块 |
| FSM 与人工等待状态衔接不足 | 失败耗尽重试后无法合法进入人工处理 | 增加 Failed -> WaitingForHuman 转换 |
| 审计只描述表结构 | 无法验证每次状态变化是否真实落库 | Scheduler 每次状态转换同时写 audit_events 并发布事件 |
| DAG 只描述拓扑排序 | 无循环拒绝和 ready 判定实现 | TaskGraph 提供 add_dependency、topological_sort、ready_tasks、has_cycle |
| 缺少验收标准 | 开发完成难以判断 | 增加 ctest 验收：独立 FSM 测试 + 完整工作流验收 |
| 外部库依赖不确定 | 在无 GTest/spdlog 环境中不可构建 | 保留 fallback logger 和 standalone tests，SQLite 可用时启用真实 DB |

## 2. 当前可交付范围

### 2.1 Domain Layer

- `TaskId`、`WorkflowId`、`ExecutorId`、`DeviceId`、`EventId`、`Timestamp`
- `TaskStateMachine`：合法状态转换校验
- `Task`：任务身份、优先级、依赖、状态、重试、checkpoint、审计时间字段
- `TaskGraph`：任务集合、依赖边、循环检测、拓扑排序、READY 任务计算

### 2.2 Infrastructure Layer

- `EventBus`：线程安全发布订阅，回调异常隔离
- `SqliteDatabase`：SQLite 可用时真实执行 SQL；否则保留 mock fallback
- `Logger`：spdlog 可选，不存在时使用标准错误输出

### 2.3 Scheduler Layer

- `SimpleScheduler`：按 DAG 依赖推进任务状态
- 状态流：`Pending -> Ready -> Running -> Completed`
- 失败流：`Running -> Failed -> Pending` 自动重试，或 `Failed -> WaitingForHuman`
- 审计：每次状态变化写入内存 audit_records、SQLite `audit_events`、EventBus

### 2.4 Application Layer

- `WorkflowManager`：应用服务入口，负责提交工作流并委托 Scheduler 执行

## 3. 模块职责边界

| 模块 | 当前职责 | 不做的事 |
|------|----------|----------|
| TaskGraph | 维护 DAG 正确性、判定 ready 任务 | 不执行任务、不访问设备 |
| TaskStateMachine | 校验状态转换是否合法 | 不决定何时转换 |
| SimpleScheduler | 驱动依赖、执行 handler、记录审计 | 不实现真实线程池和设备协议 |
| EventBus | 模块间事件广播 | 不保证全局顺序；顺序由审计时间和后续 sequence 字段承担 |
| SqliteDatabase | 执行 schema 与状态/审计 SQL | 当前不提供查询 API |
| WorkflowManager | 应用层提交与运行入口 | 当前不包含权限、人工干预和恢复编排 |

## 4. 验收流程

当前验收使用 CMake/CTest：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

验收用例：

| 测试 | 验证点 |
|------|--------|
| `Phase1_2_Standalone` | 基础类型与 FSM 转换 |
| `AcceptanceWorkflow` | `load sample -> measure sample -> archive result` 完整 DAG 执行，9 条状态转换审计，事件数量与审计数量一致 |

## 5. 下一阶段路线

1. 增加 `IDatabase` 查询接口，用于恢复、审计查询和断言落库内容。
2. 增加 ExecutorPool：线程池、设备串行执行器、取消/暂停协作协议。
3. 增加 ManualInterventionService：权限、原因必填、二次确认、人工 retry/force_complete/rollback。
4. 增加 WatchDog 与 TimeoutPolicy：软超时保存 checkpoint，硬超时取消并审计。
5. 增加 DeviceRegistry 与协议适配器：MockDevice 先扩展为可注入失败/延迟的模拟器。
6. 增加恢复启动流程：重启时将 Running 任务恢复为 Paused/WaitingForHuman，等待人工确认。

## 6. 当前工程状态

本仓库已从“纯设计阶段”推进到“核心可验收骨架”：

- 可配置
- 可编译
- 可测试
- 有端到端样本工作流
- 有 SQLite 审计落库路径
- 保留 v1.0 全量架构演进空间

*设计书版本：v1.1 | 更新日期：2026-05-07 | 当前状态：核心骨架已开发并通过验收*
