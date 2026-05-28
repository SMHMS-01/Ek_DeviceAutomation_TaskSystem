# Device Automation Task System — 条件通过隐患台账

> 用途：记录“条件通过但允许进入下一阶段”的潜在隐患。每条隐患必须包含位置、功能、影响分析、当前建议和关闭条件。

## 阶段进入原则

当前允许在条件通过状态下推进开发切片，但所有遗留项必须在本台账中持续跟踪。若隐患影响样本安全、审计完整性或恢复一致性，应提升优先级并阻塞后续发布。

## 隐患清单

| ID | 阶段 | 位置 | 功能 | 影响分析 | 当前建议 | 关闭条件 | 状态 |
|----|------|------|------|----------|----------|----------|------|
| R-001 | PHASE 1 | `src/domain/Task.h` | `CheckPoint`/`RetryPolicy` 暂内嵌 | 领域模型会膨胀，后续恢复、重试、序列化难以独立测试 | PHASE 2.2 前拆到 `CheckPoint.h/.cpp`、`RetryPolicy.h/.cpp` | 独立单测覆盖 checkpoint 序列化和 backoff 计算 | Open |
| R-002 | PHASE 1 | `src/domain/` | 缺少独立 `Event` 领域模型 | 审计事件和事件总线 payload 仍偏字符串，难以保证 schema 演进 | 引入 `Event`/`AuditEvent` 类型，避免跨层自由字符串 | Scheduler/AuditService 使用结构化事件 | Open |
| R-003 | PHASE 2 | `src/infrastructure/IDatabase.h` | DB API 仍偏底层 SQL | 调用方需要拼 SQL，存在重复和注入风险 | 保留 SQLite 直接 SQL 作为 MVP；后续增加 repository 或 statement binding | 关键写入路径不再手工拼接用户输入 | Open |
| R-004 | PHASE 2.1 | `src/infrastructure/SchemaMigration.*` | migration runner 缺失 | schema 演进无法表达多版本升级和回滚 | 已增加 migration 列表、幂等应用、失败回滚 | `PersistenceRecovery` 覆盖重复执行和失败回滚 | Closed |
| R-005 | PHASE 2 | `src/infrastructure/EventBus.*` | 当前同步发布 | 慢订阅者会阻塞调度器，异常隔离虽有但没有背压 | 短期可接受；若进入高频设备事件，优先评估 `eventpp` 或异步队列 | 事件吞吐/背压测试通过 | Open |
| R-006 | PHASE 2 | `src/infrastructure/MockDevice.h` | MockDevice 不支持延迟/故障注入 | 容错、重试和恢复场景测试不足 | PHASE 2.2 扩展 MockDeviceSimulator | 可配置 timeout、transient failure、permanent failure | Open |
| R-007 | PHASE 3.2 | `src/scheduler/ExecutorPool.*` | SimpleScheduler 非真实 ExecutorPool | 已能验证多执行器分配；取消、暂停恢复仍需任务句柄协作 | 已增加 ExecutorPool 与集成测试；取消协议另入后续风险跟踪 | `ExecutorPool` 集成测试通过 | Closed |
| R-008 | PHASE 3.3 | `src/scheduler/SchedulingPolicy.*` | 调度策略固定 | 已有策略抽象；等待时长、资源配额和限流尚未实现 | PHASE 3 后续实现 RateLimiter 和 starvation 防护 | 策略单测和限流验收通过 | Open |
| R-009 | PHASE 4 | `src/application/AuditService.*` | 恢复策略固定 `Running -> Paused` | 某些设备故障可能应进入 `WaitingForHuman` 或触发回滚 | 按任务类型、设备状态、checkpoint 安全性选择恢复目标 | 恢复矩阵与集成测试覆盖 | Open |
| R-010 | PHASE 4.2 | `src/application/InterventionService.*` | ManualInterventionService 启动切片缺少权限矩阵 | 已增加权限矩阵、二次确认和 rollback 干预 | 保持后续权限模型可扩展到真实用户/角色系统 | 高危操作权限、二次确认和审计单测通过 | Closed |
| R-011 | PHASE 5 | `src/faulttolerance/` | WatchDog/TimeoutPolicy 未实现 | 长任务或设备挂起时无法自动告警/取消/恢复 | PHASE 5 前置实现任务级 watchdog | timeout 集成测试通过 | Open |
| R-012 | 工程 | `弃用资料/test_phase_1_2` | 历史二进制仍被版本库跟踪但已归档 | 仓库体积与可移植性受影响 | 后续确认不需保留后，从 Git 中删除并改由 build 生成 | 审批后删除跟踪二进制 | Open |
| R-013 | PHASE 3.2 | `src/scheduler/ExecutorPool.cpp` | ExecutorPool 当前基于 `std::async` MVP | 可完成多执行器验证，但还不是固定大小线程池，1000 任务压测和 graceful shutdown 语义不足 | 引入 `BS::thread_pool` 或 Taskflow executor 适配，保留现有 `IExecutor` 窄接口 | 高压并发、关闭流程和资源上限测试通过 | Open |
| R-014 | PHASE 4.2 | `src/application/DeviceStateReconciler.*` | 缺少 DeviceStateReconciler | 已增加只读协调层，可输出 Consistent/DeviceAhead/DeviceBehind/Unknown | 下一步将协调结果接入恢复策略，R-009 继续跟踪 | 重启后 Consistent/DeviceAhead/DeviceBehind/Unknown 矩阵测试通过 | Closed |

## 开源库复用原则

结合设计文档第 14 节，后续实现优先复用成熟库，避免重复造轮子：

| 功能 | 优先选项 | 当前策略 |
|------|----------|----------|
| DAG/任务编排 | Taskflow | SimpleScheduler 仅作为 MVP；复杂 DAG/条件分支进入 PHASE 5 前评估 Taskflow |
| 状态机 | Boost.SML | 当前手写 FSM 便于早期验证；状态爆炸前评估 Boost.SML |
| 线程池 | BS::thread_pool | 当前 ExecutorPool 用 `std::async` 作为 PHASE 3.2 MVP；进入压测前优先替换或适配 |
| 事件总线 | eventpp | 当前 EventBus 为轻量同步 MVP；高吞吐/过滤需求出现时替换或适配 |
| SQLite C++ 封装 | SQLiteCpp | 当前直接使用 sqlite3 C API；查询/事务复杂化后评估 SQLiteCpp |
| 日志 | spdlog | 已做可选集成与 fallback |

---

**最后更新**：2026-05-28
**当前策略**：PHASE 3.3 已进入 beta 可用状态；高风险项必须在发布候选前关闭或降级为明确可接受约束。
