# Device Automation Task System — 阶段验收与审批表 v1.2

> 用途：这是测试验证和审批表。执行计划写在 `MASTER_ROADMAP.md`，架构设计写在设计书，条件通过隐患写在 `RISK_REGISTER.md`。

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
| 条件通过 | 可进入下一阶段，但必须在 `RISK_REGISTER.md` 跟踪遗留项 |
| 不通过 | 阻塞，必须修复后重审 |

## 1.1 阶段标志对应关系

| 标志 | 对应文档 | 说明 |
|------|----------|------|
| PHASE N | `MASTER_ROADMAP.md` / 本文档 | 顶层阶段，例如 PHASE 2 = Infrastructure Core |
| PHASE N.M | `MASTER_ROADMAP.md` / 本文档 | 阶段内开发切片，例如 PHASE 2.1 = Persistence Recovery |
| Risk ID | `RISK_REGISTER.md` | 条件通过隐患编号 |
| Design Section | 设计书 | 架构来源章节，例如第 14 节开源库选型 |

## 2. 当前阶段汇总

| 阶段 | 名称 | 状态 | 结论 |
|------|------|------|------|
| Phase 0 | 文档职责整理 | 完成 | 通过 |
| Phase 1 | Domain Core | 基本完成 | 条件通过 |
| Phase 2 | Infrastructure Core | PHASE 2.1 完成 | 通过，非阻塞隐患入台账 |
| Phase 3 | Scheduler & Executors | 已进入，PHASE 3.1 启动 | 进行中 |
| Phase 4 | Application Services | 仅提前完成切片 | 未进入 |
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

遗留风险：见 `RISK_REGISTER.md` 的 R-003、R-004、R-005、R-006、R-011。

审批结论：条件通过

## 6. PHASE 2.1 — Persistence Recovery

交付物：

- `src/infrastructure/IDatabase.h` query/transaction API
- `src/infrastructure/SqliteDatabase.h/.cpp` query/transaction implementation
- `src/infrastructure/SchemaMigration.h/.cpp`
- `src/application/AuditService.h/.cpp`
- `tests/integration/test_persistence_recovery.cpp`

验收项：

| 检查项 | 结果 |
|--------|------|
| SQLite SELECT 查询返回行集合 | 通过 |
| schema_migrations 初始化 | 通过 |
| Migration runner 幂等执行 | 通过 |
| Migration runner 失败回滚 | 通过 |
| SQLite transaction commit/rollback | 通过 |
| 按 task_id 查询审计事件 | 通过 |
| 按 workflow_id 查询审计事件 | 通过 |
| 审计事件重放任务最终状态 | 通过 |
| Running 任务恢复为 Paused | 通过 |
| 写入 SystemRecovered 审计事件 | 通过 |

测试证据：

```bash
cmake --build build
ctest --test-dir build --output-on-failure
python3 scripts/check_includes.py
```

结果：

```text
4/4 tests passed
include rules pass
```

审批结论：通过，可进入 PHASE 3。R-004 已关闭；R-003、R-009 作为非阻塞隐患保留。

## 7. PHASE 3 — Scheduler & Executors

当前状态：已进入 PHASE 3，启动 `PHASE 3.1 Executor Contract`。

### 8.1 已有 Scheduler MVP

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

### 8.2 PHASE 3.1 — Executor Contract 启动切片

交付物：

- `src/scheduler/IExecutor.h`
- `src/scheduler/InlineExecutor.h/.cpp`
- `src/tools/TaskSystemCli.cpp`
- `tests/fixtures/sample_workflow_linear.csv`
- `CliSmoke` 集成测试

验收项：

| 检查项 | 结果 |
|--------|------|
| 主程序 `device_automation_task_system` 可编译 | 通过 |
| 主程序可读取样例工作流 CSV | 通过 |
| InlineExecutor 可执行任务 handler | 通过 |
| CLI 端到端 smoke test | 通过 |

测试证据：

```bash
ctest --test-dir build --output-on-failure
ctest --test-dir build-release --output-on-failure
```

结果：

```text
Debug: 4/4 tests passed
Release: 4/4 tests passed
```

审批结论：PHASE 3 进行中，PHASE 3.1 启动切片通过。

## 8. PHASE 4 — Application Services

PHASE 4 尚未正式进入。当前只有提前完成、服务于 PHASE 2.1/PHASE 3 验收的应用切片：

- `src/application/WorkflowManager.h/.cpp`
- `src/application/AuditService.h/.cpp`

审批结论：未进入；风险见 R-010。

## 9. Git 与发布审批

当前检查结果：

| 项目 | 结果 |
|------|------|
| 当前分支 | `develop` |
| 远程默认分支 | `main` |
| 本地 `develop` upstream | 已配置，跟踪 `origin/develop` |
| 远程 `main` | 已跟踪 |
| 当前标签 | `v1.0.0-design`, `v1.0.0-design-dev`, `v1.0.0-roadmap`, `v1.0.0-ready`, `v1.0.0-phase-1.2-complete` |
| 本地提交 | 以当前 HEAD 为准 |
| 本地新标签 | `v1.1.0-core-scaffold` |
| 远程推送 | 用户已授权每次提交后推送 |

发布建议：

- 每次本地提交后推送 `develop`
- 阶段标签只在审批通过的稳定阶段创建
- 暂不合并到 `main`；PHASE 3 执行器主干验收后再准备 release 分支

审批结论：按用户授权，后续提交同步推送远程。

## 10. 下一阶段准入条件

PHASE 3 当前准入状态：

- 当前工作区可构建
- 当前测试全部通过
- include rule 检查通过或记录例外
- PHASE 2.1 非阻塞隐患已记录在 `RISK_REGISTER.md`
- `develop` 当前已作为 PHASE 3 起点

---

**文档版本**：v1.2
**最后更新**：2026-05-08  
**维护者**：hms03 / Codex
