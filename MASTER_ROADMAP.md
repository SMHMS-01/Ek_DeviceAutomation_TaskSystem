# Device Automation Task System — 作战书 v1.1

> 用途：这是执行计划，不是架构设计书。架构细节以 `DeviceAutomation_TaskSystem_DesignDoc.md` 和 `DeviceAutomation_TaskSystem_DesignDoc_v1.1_Optimized.md` 为准；验收证据以 `PHASE_APPROVAL.md` 为准。

## 1. 当前态势

| 项目 | 状态 |
|------|------|
| 当前分支 | `develop` |
| 远程仓库 | `origin=https://github.com/SMHMS-01/Ek_DeviceAutomation_TaskSystem.git` |
| 远程跟踪 | `main` 已跟踪；`develop` 尚未建立 upstream |
| 最新设计基线 | v1.1 Optimized |
| 当前开发阶段 | DEV PHASE 2.1：Persistence Recovery MVP |
| 当前验收结果 | `ctest --test-dir build --output-on-failure` 通过，3/3 |
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

状态：部分完成

已完成：
- Logger fallback
- EventBus
- IDatabase
- SqliteDatabase
- IDevice/MockDevice 基础接口

遗留：
- DB 查询 API
- 事务封装
- 数据库 migration 版本表
- MockDevice 故障注入
- WatchDog 实现
- PluginLoader 实现

### Phase 3 — Scheduler MVP

目标：让工作流可以从应用层提交、调度、审计并完成。

状态：MVP 完成

已完成：
- SimpleScheduler
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

### 当前阶段：DEV PHASE 2.1 — Persistence Recovery

目标：让系统不仅能写入审计，还能查询、恢复、验证落库状态。

执行队列：

1. 扩展 `IDatabase`
   - 已完成：增加 query API
   - 待完成：增加事务 helper
   - 已完成：增加基础 schema migration 入口

2. 扩展 `SqliteDatabase`
   - 已完成：支持 `SELECT` 返回行集合
   - 已完成：schema 初始化加 `schema_migrations`
   - 已完成：audit_events/tasks 查询测试

3. 增加 `AuditService`
   - 已完成：查询 task 审计流
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

---

**文档版本**：v1.1  
**最后更新**：2026-05-08  
**当前阶段**：DEV PHASE 2.1 MVP 完成，准备扩展事务、迁移和恢复策略
