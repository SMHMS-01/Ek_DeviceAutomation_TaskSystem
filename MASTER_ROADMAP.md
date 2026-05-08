# Device Automation Task System — 作战书 v1.1

> 用途：这是执行计划，不是架构设计书。架构细节以 `DeviceAutomation_TaskSystem_DesignDoc.md` 和 `DeviceAutomation_TaskSystem_DesignDoc_v1.1_Optimized.md` 为准；验收证据以 `PHASE_APPROVAL.md` 为准；条件通过隐患以 `RISK_REGISTER.md` 为准。

## 0. 文档标志对应关系

| 标志 | 所属文档 | 含义 | 对应关系 |
|------|----------|------|----------|
| Design Section | `DeviceAutomation_TaskSystem_DesignDoc.md` | 原始架构章节 | 例如第 7 节对应持久化，第 14 节对应开源库选型 |
| Optimized Design v1.1 | `DeviceAutomation_TaskSystem_DesignDoc_v1.1_Optimized.md` | 工程化后的设计基线 | 将原始设计映射为可交付 MVP 和后续路线 |
| DEV PHASE | `MASTER_ROADMAP.md` | 作战执行阶段 | 与 PHASE 审批阶段同号，允许细分如 `PHASE 2.1` |
| Approval Phase | `PHASE_APPROVAL.md` | 验收和审批阶段 | 记录测试证据、审批结论和准入状态 |
| Risk ID | `RISK_REGISTER.md` | 条件通过隐患 | 记录位置、功能、影响、建议和关闭条件 |

阶段命名规则：`PHASE 2.1` 表示 Design/Approval 的 `PHASE 2 Infrastructure Core` 下的第 1 个开发切片 `Persistence Recovery`。作战书、审批表和隐患台账必须使用同一阶段编号。

## 1. 当前态势

| 项目 | 状态 |
|------|------|
| 当前分支 | `develop` |
| 远程仓库 | `origin=https://github.com/SMHMS-01/Ek_DeviceAutomation_TaskSystem.git` |
| 远程跟踪 | `main` 已跟踪；`develop` 尚未建立 upstream |
| 最新设计基线 | v1.1 Optimized |
| 当前开发阶段 | PHASE 3.1：Executor Contract 启动 |
| 当前验收结果 | Debug/Release `ctest` 均通过，4/4 |
| 本地提交 | `feat: implement v1.1 core workflow scaffold`，当前 HEAD |
| 本地标签 | `v1.1.0-core-scaffold` |
| 远程推送 | 被安全策略拦截，需用户知晓外部数据导出风险后再次明确批准 |
| 已归档资料 | 根目录旧构建产物 `test_phase_1_2` 已移入 `弃用资料/` |

## 2. DEV PHASE 设计

### Phase 0 — 基线整理

目标：确认设计、作战书、审批表三者职责清晰。

状态：完成

交付物：
- `DeviceAutomation_TaskSystem_DesignDoc_v1.1_Optimized.md`
- `MASTER_ROADMAP.md`
- `PHASE_APPROVAL.md`

### Phase 1 — Domain Core

目标：建立任务生命周期和 DAG 正确性。

状态：基本完成

已完成：
- ID 与时间类型
- Priority
- TaskStateMachine
- Task
- TaskGraph
- 循环依赖拒绝
- Ready task 计算

遗留：
- 独立 `Event` 领域模型
- CheckPoint/RetryPolicy 从 `Task.h` 拆成独立模块
- JSON/YAML 序列化

### Phase 2 — Infrastructure Core

目标：建立可运行的基础设施主干。

状态：PHASE 2.1 完成，保留非阻塞隐患进入 PHASE 3 台账

已完成：
- Logger fallback
- EventBus
- IDatabase
- SqliteDatabase
- IDevice/MockDevice 基础接口
- DB 查询 API
- 事务接口
- 可演进 migration runner
- migration 幂等执行和失败回滚验收
- task/workflow 审计查询
- Running 任务恢复为 Paused

遗留：
- MockDevice 故障注入
- WatchDog 实现
- PluginLoader 实现

### Phase 3 — Scheduler & Executors

目标：让工作流可以分配给执行器，逐步支持线程池、设备串行执行和调度策略。

状态：PHASE 3.1 已启动

已完成：
- SimpleScheduler
- IExecutor
- InlineExecutor
- 主程序 smoke 入口 `device_automation_task_system`
- 样例测试数据 `tests/fixtures/sample_workflow_linear.csv`
- `Pending -> Ready -> Running -> Completed`
- `Failed -> Pending` 自动重试
- `Failed -> WaitingForHuman` 人工等待
- SQLite audit_events 写入
- EventBus 状态事件发布

遗留：
- ExecutorPool
- 线程池/协程/设备执行器
- RateLimiter
- SchedulingPolicy 抽象
- 优先级等待时长排序细化

### Phase 4 — Application MVP

目标：提供应用服务入口。

状态：MVP 完成

已完成：
- WorkflowManager
- submit_and_run 集成入口

遗留：
- ManualInterventionService
- AuditService
- HealthMonitor
- 权限与高危操作二次确认

### Phase 5 — Fault Tolerance

目标：实现可恢复、可回滚、可降级。

状态：未开始

下一步重点：
- CheckPointManager
- RetryEngine
- CircuitBreaker
- TimeoutPolicy
- 启动恢复：Running -> Paused/WaitingForHuman

## 3. 下一阶段作战目标

### 当前阶段：PHASE 2.1 — Persistence Recovery

目标：让系统不仅能写入审计，还能查询、恢复、验证落库状态。

状态：完成，可进入 PHASE 3。

执行队列：

1. 扩展 `IDatabase`
   - 已完成：增加 query API
   - 已完成：增加事务 helper
   - 已完成：增加 schema migration runner

2. 扩展 `SqliteDatabase`
   - 已完成：支持 `SELECT` 返回行集合
   - 已完成：schema 初始化加 `schema_migrations`
   - 已完成：audit_events/tasks 查询测试
   - 已完成：transaction commit/rollback 验收

3. 增加 `AuditService`
   - 已完成：查询 task 审计流
   - 已完成：查询 workflow 审计流
   - 已完成：按事件重放任务状态
   - 已完成：验证状态重建结果与 tasks 表一致

4. 增加恢复流程
   - 已完成：启动时扫描 Running 任务
   - 已完成：恢复为 Paused
   - 已完成：记录 `SystemRecovered` 审计事件

5. 增加验收测试
   - 已完成：审计落库查询
   - 已完成：状态重放
   - 已完成：模拟崩溃恢复
   - 已完成：migration 幂等和失败回滚

### 当前阶段：PHASE 3 — Scheduler & Executors

进入条件：

- PHASE 2.1 验收命令全部通过
- 非阻塞隐患已记录到 `RISK_REGISTER.md`
- 持久化恢复能力可供 PHASE 3 调度器/执行器使用

首批任务：

1. 已完成：设计 `IExecutor` 接口和执行结果协议。
2. 已完成：增加 `InlineExecutor`，作为 PHASE 3 执行器契约 smoke slice。
3. 已完成：增加可编译主程序 `device_automation_task_system`。
4. 已完成：准备样例工作流数据 `tests/fixtures/sample_workflow_linear.csv`。
5. 下一步：引入 ExecutorPool，优先评估 `BS::thread_pool`，避免手写线程池。
6. 下一步：抽象 `SchedulingPolicy`，为 PriorityFirst 和 DeviceAffinity 留扩展点。
7. 下一步：评估 `Taskflow` 是否作为完整 DAG 编排引擎，SimpleScheduler 保留为 MVP/测试适配层。
8. 下一步：增加执行器集成测试：成功、失败、重试、取消、设备串行执行。

主程序当前可用性：

```bash
cmake --build build
./build/bin/device_automation_task_system tests/fixtures/sample_workflow_linear.csv /tmp/device_automation_cli.sqlite
```

当前主程序是 PHASE 3.1 smoke 版本，可编译、可运行、可读取样例 CSV 并完成一条线性工作流；生产可用主程序预计在 PHASE 3.3 完成 ExecutorPool、SchedulingPolicy 和设备串行执行器后进入 beta。

验收命令：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
python3 scripts/check_includes.py
```

## 4. Git 作战规则

当前分支策略：

| 分支 | 用途 |
|------|------|
| `main` | 稳定发布分支 |
| `develop` | 当前开发集成分支 |
| `feature/*` | 大功能开发分支 |
| `release/*` | 发布候选分支 |

本轮建议 Git 动作：

1. 保持当前工作在 `develop`
2. 提交当前 v1.1 核心骨架与文档整理
3. 为本地 `develop` 建立远程 upstream：`origin/develop`
4. 创建标签：`v1.1.0-core-scaffold`
5. 推送 `develop` 和标签

提交信息建议：

```bash
git add .
git commit -m "feat: implement v1.1 core workflow scaffold"
git push -u origin develop
git tag v1.1.0-core-scaffold
git push origin v1.1.0-core-scaffold
```

## 5. 作战纪律

- `MASTER_ROADMAP.md` 只写计划、当前态势、下一步动作。
- `PHASE_APPROVAL.md` 只写测试证据、审批结果、风险和签署。
- 设计细节进入设计书，不塞进作战书。
- 构建产物不放根目录；历史二进制统一放 `弃用资料/` 或从版本库删除。
- 每个阶段必须有可运行验收命令，不能只写“已完成”。
- 条件通过的隐患进入 `RISK_REGISTER.md`，不得只散落在聊天或备注里。
- 设计文档第 14 节已有成熟开源库选型；进入完整实现前必须先评估复用，MVP 手写实现必须标明后续替换点。

---

**文档版本**：v1.1  
**最后更新**：2026-05-08  
**当前阶段**：PHASE 3.1 Executor Contract 已启动，主程序 smoke 版本可编译运行
