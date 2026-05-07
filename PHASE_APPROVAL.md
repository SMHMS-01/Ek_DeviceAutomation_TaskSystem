# Device Automation Task System — 阶段验收与审批表 v1.1

> 用途：这是测试验证、审批和风险台账。执行计划写在 `MASTER_ROADMAP.md`，架构设计写在设计书。

## 1. 审批规则

每个阶段必须满足：

- 有明确交付物
- 有可复现验收命令
- 有测试输出记录
- 有遗留风险说明
- 有审批结论

审批状态：

| 状态 | 含义 |
|------|------|
| 通过 | 可进入下一阶段 |
| 条件通过 | 可进入下一阶段，但必须跟踪遗留项 |
| 不通过 | 阻塞，必须修复后重审 |

## 2. 当前阶段汇总

| 阶段 | 名称 | 状态 | 结论 |
|------|------|------|------|
| Phase 0 | 文档职责整理 | 完成 | 通过 |
| Phase 1 | Domain Core | 基本完成 | 条件通过 |
| Phase 2 | Infrastructure Core | Persistence MVP 完成 | 条件通过 |
| Phase 3 | Scheduler MVP | MVP 完成 | 条件通过 |
| Phase 4 | Application MVP | MVP 完成 | 条件通过 |
| Phase 5 | Fault Tolerance | 未开始 | 未审批 |

## 3. Phase 0 — 文档职责整理

交付物：

- `DeviceAutomation_TaskSystem_DesignDoc_v1.1_Optimized.md`
- `MASTER_ROADMAP.md`
- `PHASE_APPROVAL.md`

验收项：

| 检查项 | 结果 |
|--------|------|
| 设计书和作战书职责分离 | 通过 |
| 作战书只保留执行计划 | 通过 |
| 审批表只保留测试与审批证据 | 通过 |
| 过期构建产物归档 | 通过 |

审批结论：通过

## 4. Phase 1 — Domain Core

交付物：

- `src/domain/Types.h/.cpp`
- `src/domain/Priority.h`
- `src/domain/TaskState.h`
- `src/domain/TaskStateMachine.h/.cpp`
- `src/domain/Task.h/.cpp`
- `src/domain/TaskGraph.h/.cpp`

验收项：

| 检查项 | 结果 |
|--------|------|
| ID/时间类型可用 | 通过 |
| FSM 合法/非法状态转换验证 | 通过 |
| Task 基础模型可用 | 通过 |
| TaskGraph 拓扑排序 | 通过 |
| 循环依赖拒绝 | 通过 |
| Ready task 计算 | 通过 |

测试证据：

```bash
ctest --test-dir build --output-on-failure
```

结果：

```text
2/2 tests passed
```

遗留风险：

- CheckPoint/RetryPolicy 目前仍内嵌在 `Task.h`，后续应拆为独立模块。
- 尚未实现领域事件 `Event` 模型。
- 尚未实现序列化 round-trip 测试。

审批结论：条件通过

## 5. Phase 2 — Infrastructure Core

交付物：

- `src/infrastructure/IDatabase.h`
- `src/infrastructure/SqliteDatabase.h/.cpp`
- `src/infrastructure/EventBus.h/.cpp`
- `src/infrastructure/Logger.h`
- `src/infrastructure/IDevice.h`
- `src/infrastructure/MockDevice.h`

验收项：

| 检查项 | 结果 |
|--------|------|
| SQLite 可打开并执行 schema SQL | 通过 |
| fallback logger 可用 | 通过 |
| EventBus 可发布订阅 | 通过 |
| Scheduler 可写 tasks/audit_events | 通过 |

测试证据：

```bash
ctest --test-dir build --output-on-failure
```

结果：

```text
2/2 tests passed
```

遗留风险：

- `IDatabase` 还没有 query API，无法直接断言落库内容。
- SQLite migration 尚未实现。
- EventBus 当前为同步发布，后续需要异步分发和背压策略。
- WatchDog、PluginLoader 仍是接口/占位。

审批结论：条件通过

## 6. Phase 3 — Scheduler MVP

交付物：

- `src/scheduler/SimpleScheduler.h/.cpp`
- `tests/integration/test_acceptance_workflow.cpp`

验收项：

| 检查项 | 结果 |
|--------|------|
| DAG 顺序执行 | 通过 |
| 状态转换审计 | 通过 |
| EventBus 状态事件发布 | 通过 |
| 失败后进入 WaitingForHuman 的 FSM 路径 | 通过 |

验收工作流：

```text
load sample -> measure sample -> archive result
```

验收结果：

```text
AcceptanceWorkflow passed
9 audited state transitions
```

遗留风险：

- 尚未实现 ExecutorPool。
- 尚未实现真实并发调度。
- 尚未实现 RateLimiter 和 SchedulingPolicy。
- 当前 handler 为测试注入函数，不是真实设备任务执行器。

审批结论：条件通过

## 7. Phase 4 — Application MVP

交付物：

- `src/application/WorkflowManager.h/.cpp`

验收项：

| 检查项 | 结果 |
|--------|------|
| 应用层可提交工作流 | 通过 |
| 应用层可委托 Scheduler 执行 | 通过 |
| 集成验收走 WorkflowManager 入口 | 通过 |

遗留风险：

- ManualInterventionService 尚未实现。
- AuditService 尚未实现。
- HealthMonitor 尚未实现。
- 权限、审批、二次确认尚未实现。

审批结论：条件通过

## 8. DEV PHASE 2.1 — Persistence Recovery MVP

交付物：

- `src/infrastructure/IDatabase.h` query API
- `src/infrastructure/SqliteDatabase.h/.cpp` query implementation
- `src/application/AuditService.h/.cpp`
- `tests/integration/test_persistence_recovery.cpp`

验收项：

| 检查项 | 结果 |
|--------|------|
| SQLite SELECT 查询返回行集合 | 通过 |
| schema_migrations 初始化 | 通过 |
| 按 task_id 查询审计事件 | 通过 |
| 按审计事件重放任务最终状态 | 通过 |
| 扫描 Running 任务并恢复为 Paused | 通过 |
| 写入 SystemRecovered 审计事件 | 通过 |

测试证据：

```bash
cmake --build build
ctest --test-dir build --output-on-failure
python3 scripts/check_includes.py
```

结果：

```text
3/3 tests passed
include rules pass
```

遗留风险：

- 恢复策略当前固定为 `Running -> Paused`，后续需按任务类型/设备安全态决定是否进入 `WaitingForHuman`。
- `IDatabase` 仍缺少显式 transaction helper。
- migration 当前只有 version=1 的基础记录，还没有可演进 migration runner。
- `AuditService` 当前支持 task 维度查询，workflow 维度查询待补。

审批结论：条件通过

## 9. Git 与发布审批

当前检查结果：

| 项目 | 结果 |
|------|------|
| 当前分支 | `develop` |
| 远程默认分支 | `main` |
| 本地 `develop` upstream | 未配置 |
| 远程 `main` | 已跟踪 |
| 当前标签 | `v1.0.0-design`, `v1.0.0-design-dev`, `v1.0.0-roadmap`, `v1.0.0-ready`, `v1.0.0-phase-1.2-complete` |
| 本地提交 | `feat: implement v1.1 core workflow scaffold`，当前 HEAD |
| 本地新标签 | `v1.1.0-core-scaffold` |
| 远程推送 | 因外部数据导出安全策略被拦截，等待用户明确再次批准 |

发布建议：

- 当前 v1.1 变更通过测试后提交到 `develop`
- 推送 `develop` 并建立 upstream
- 创建并推送 `v1.1.0-core-scaffold` 标签
- 暂不合并到 `main`，等 DEV PHASE 2.1 完成数据库查询和恢复验收后再准备 release 分支

审批结论：本地提交与本地标签通过；远程推送待用户明确再次批准后复核

## 10. 下一阶段准入条件

进入 DEV PHASE 2.1 前必须满足：

- 当前工作区可构建
- 当前测试全部通过
- include rule 检查通过或记录例外
- `develop` 已推送远程
- `v1.1.0-core-scaffold` 标签已推送

---

**文档版本**：v1.1  
**最后更新**：2026-05-08  
**维护者**：hms03 / Codex
