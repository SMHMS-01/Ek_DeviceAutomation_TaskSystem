# 快速开始 — v1.1 Core Scaffold

## 当前状态

本项目已从纯设计阶段推进到核心可验收骨架：

- Domain：Task、TaskGraph、FSM、Priority、ID/时间类型
- Infrastructure：EventBus、SqliteDatabase、Logger fallback、设备接口基础
- Scheduler：SimpleScheduler，可按 DAG 推进状态并写审计
- Application：WorkflowManager，作为工作流提交入口；AuditService 可查询审计、重放任务状态并恢复中断任务
- Acceptance：端到端工作流测试通过

完整执行计划见 [MASTER_ROADMAP.md](MASTER_ROADMAP.md)。  
测试和审批证据见 [PHASE_APPROVAL.md](PHASE_APPROVAL.md)。  
优化设计基线见 [DeviceAutomation_TaskSystem_DesignDoc_v1.1_Optimized.md](DeviceAutomation_TaskSystem_DesignDoc_v1.1_Optimized.md)。

## 构建与验收

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

当前通过项：

```text
Phase1_2_Standalone
AcceptanceWorkflow
PersistenceRecovery
```

## 代码规则检查

```bash
python3 scripts/check_includes.py
```

## Git 工作流

当前建议：

```bash
git checkout develop
git status --short --branch
git add .
git commit -m "feat: implement v1.1 core workflow scaffold"
git push -u origin develop
git tag v1.1.0-core-scaffold
git push origin v1.1.0-core-scaffold
```

`main` 保持稳定发布分支；`develop` 作为开发集成分支。当前不建议直接合并 `main`，等 DEV PHASE 2.1 的数据库查询和恢复验收完成后再准备 release 分支。

## 下一阶段

DEV PHASE 2.1 — Persistence Recovery 已完成 MVP，下一步继续加固：

1. 增加显式事务 helper
2. 增加可演进 migration runner
3. 扩展 workflow 维度审计查询
4. 增加更细的恢复策略：Paused / WaitingForHuman / Rollback
5. 增加故障注入型恢复验收
