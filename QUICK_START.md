# 快速开始 — v1.1 Core Scaffold

## 当前状态

本项目已从纯设计阶段推进到核心可验收骨架：

- Domain：Task、TaskGraph、FSM、Priority、ID/时间类型
- Infrastructure：EventBus、SqliteDatabase、Logger fallback、设备接口基础
- Scheduler：SimpleScheduler，可按 DAG 推进状态并写审计
- Executor：IExecutor、InlineExecutor、ExecutorPool、SchedulingPolicy、DeviceExecutor 已完成 PHASE 3.3 beta 主干
- Application：WorkflowManager，作为工作流提交入口；AuditService 可查询审计、重放任务状态并恢复中断任务；InterventionService 支持权限/确认/rollback 人工干预审计；DeviceStateReconciler 支持设备状态协调矩阵
- Main：`device_automation_task_system` 可编译运行，支持样例 CSV 工作流
- Acceptance：端到端工作流、持久化恢复、ExecutorPool、InterventionService、CLI smoke 测试通过

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
ExecutorPool
InterventionService
DeviceStateReconciler
CliSmoke
```

## 运行主程序

```bash
cmake --build build
./build/bin/device_automation_task_system tests/fixtures/sample_workflow_linear.csv /tmp/device_automation_cli.sqlite
```

预期输出：

```text
workflow=cli-sample-workflow state=Completed tasks=3 audits=9
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
git commit -m "feat: add executor pool and intervention service"
git push origin develop
```

`main` 保持稳定发布分支；`develop` 作为开发集成分支。PHASE 4.2 已完成，当前主程序进入 beta 可用状态；合并 `main` 仍建议等恢复策略接入、超时和取消闭环后再准备 release 分支。

## 下一阶段

PHASE 4 — Application Services 继续推进：

1. 已有 `InterventionService` 的权限矩阵、二次确认和 rollback 干预。
2. 已有 `DeviceStateReconciler` 的 Consistent/DeviceAhead/DeviceBehind/Unknown 矩阵。
3. 下一步将 DeviceStateReconciler 接入启动恢复策略。
4. 进入高并发验证前优先评估 Taskflow、BS::thread_pool、eventpp，避免重复造轮子。
