# 快速参考 — 开发启动指南

## 📋 项目部署完成清单

✅ **设计文档** — 完整的系统设计书（中文）  
✅ **项目文档** — README.md + AGENTS.md + MASTER_ROADMAP.md  
✅ **开发路线图** — 7个阶段，详细的验收标准与 Benchmark 指标  
✅ **代码规范** — 命名约定、代码风格、头文件包含规则  
✅ **工程规范** — 分层架构、CI/CD 流程、测试框架  
✅ **构建配置** — CMake 全栈配置，支持 Debug/Release/Coverage  
✅ **自动化检查** — 包含规则检查、代码格式检查、静态分析  
✅ **测试框架** — Google Test + Google Benchmark，单元/集成/压力/基准测试  
✅ **GitHub Actions** — 完整 CI Pipeline

---

## 🚀 快速开始

### 1. 克隆并初始化

```bash
git clone <repo>
cd device-automation-task-system
git checkout develop  # 选择开发分支
```

### 2. 构建项目

```bash
# 简单方式（使用脚本）
bash scripts/build.sh Debug   # Debug 构建 + 测试 + 覆盖率
bash scripts/build.sh Release # Release 优化构建

# 或手动 CMake
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
make -j$(nproc)
ctest --output-on-failure
```

### 3. 代码检查

```bash
# 格式检查
clang-format -i $(find src tests -name '*.cpp' -o -name '*.h')

# 包含规则检查（自动 CI 检查）
python3 scripts/check_includes.py

# 静态分析
clang-tidy src/**/*.cpp -- -std=c++20
```

---

## 📁 项目目录结构

```
src/                          # 源代码（5层严格分层）
├── domain/                   # Layer 0: 纯领域模型
├── infrastructure/           # Layer 1: 基础设施（数据库、事件总线）
├── scheduler/                # Layer 2: 调度与执行
├── faulttolerance/          # Layer 3: 容错与恢复
└── application/             # Layer 4: 应用服务

tests/                        # 测试（镜像 src 结构）
├── unit/                    # 单元测试
├── integration/             # 集成测试
├── stress/                  # 压力测试
├── benchmark/               # 性能基准
└── fixtures/                # 测试数据与模拟对象

docs/                        # 文档（待创建）
examples/                    # 示例代码（待创建）
scripts/                     # 工具脚本
├── build.sh                # 构建脚本
└── check_includes.py       # 包含规则检查
```

---

## 🎯 分阶段开发计划

| Phase | 周期 | 关键交付物 | 验收标准 |
|-------|------|----------|--------|
| **Phase 1** | W1-3 | Domain Layer | 100+ 单测，95%+ 覆盖率 |
| **Phase 2** | W4-6 | Infrastructure | 数据库、EventBus、设备接口 |
| **Phase 3** | W7-10 | Scheduler | DAG 引擎、多种执行器 |
| **Phase 4** | W11-13 | FaultTolerance | Checkpoint、重试、回滚、熔断 |
| **Phase 5** | W14-16 | Application | WorkflowManager、审计服务 |
| **Phase 6** | W17-19 | Integration | E2E 测试、压力测试、优化 |
| **Phase 7** | W20-22 | Release | 文档、示例、v1.0.0 发布 |

详见 [MASTER_ROADMAP.md](MASTER_ROADMAP.md) Section 1

---

## 💡 命名与代码风格

| 元素 | 规范 | 示例 |
|------|------|------|
| 文件名 | PascalCase（模块名=文件名） | `TaskStateMachine.h` |
| 类名 | PascalCase | `class Scheduler` |
| 函数 | camelCase 公开，snake_case_ 私有 | `void submit()`, `void dispatch_loop_()` |
| 常量 | UPPER_SNAKE_CASE | `const int MAX_RETRIES` |
| 成员 | snake_case_（私有） | `int max_retries_` |
| 禁止 | ❌ 匈牙利、缩写、I前缀 | ❌ `m_taskId`, `sch`, `IExecutor` |

详见 [MASTER_ROADMAP.md](MASTER_ROADMAP.md) Section 3

---

## 🔒 分层隔离规则

```
Layer 0 (Domain)          ─ 纯领域模型，仅标准库 + nlohmann::json
  ↓
Layer 1 (Infrastructure)  ─ 依赖 Layer 0，可依赖外部库
  ↓
Layer 2 (Scheduler)       ─ 依赖 Layer 0-1，禁止依赖 Layer 3-4
  ↓
Layer 3 (FaultTolerance)  ─ 依赖 Layer 0-2，禁止依赖 Layer 4
  ↓
Layer 4 (Application)     ─ 依赖所有下层
```

**CI 自动检查**: 违反分层的 PR 自动拒绝（脚本 `check_includes.py`）

---

## 🧪 测试策略

### 单元测试

```cpp
// 文件：tests/unit/domain/test_task.cpp
#include <gtest/gtest.h>
#include "domain/Task.h"

TEST(TaskTest, TaskIdIsUnique) {
    auto task1 = create_sample_task();
    auto task2 = create_sample_task();
    EXPECT_NE(task1.id, task2.id);
}
```

**覆盖率要求**:
- Domain: 95%+
- Infrastructure: 90%+
- Scheduler: 90%+
- FaultTolerance: 95%+
- Application: 85%+

### 性能 Benchmark

```cpp
// 文件：tests/benchmark/scheduler_benchmark.cpp
BENCHMARK_F(SchedulerBenchmark, HundredIndependentTasks)...
```

**性能目标**:
- 单任务提交延迟: < 10ms
- DAG 拓扑排序 (1000 节点): < 50ms
- 调度器吞吐: > 10,000 任务/秒

### 构建覆盖率报告

```bash
cmake -DENABLE_COVERAGE=ON ..
make
ctest
lcov --directory . --capture --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
open coverage_report/index.html
```

---

## 📊 CI/CD 流程（GitHub Actions）

**自动触发**:
- Push 到 main/develop 分支
- Pull Request 到 main/develop

**检查项**:
1. ✅ 包含规则检查 (`check_includes.py`)
2. ✅ 代码格式检查 (`clang-format`)
3. ✅ 构建 (GCC-13 + Clang-18, Debug + Release)
4. ✅ 单元测试
5. ✅ 代码覆盖率（Debug 模式）
6. ✅ 静态分析 (`clang-tidy`)
7. ✅ 文档存在性检查

**PR 审查清单**:
- ✅ CI 全部通过
- ✅ 代码覆盖率 > 阈值
- ✅ 新代码有单元测试
- ✅ 公共 API 有 Doxygen 注释
- ✅ CHANGELOG.md 已更新

---

## 🔀 分支策略

| 分支 | 用途 | 版本 |
|------|------|------|
| `main` | 生产发布（仅 merge PR） | v1.0.0, v1.1.0 |
| `develop` | 开发集成 | v1.x.x-dev |
| `feature/*` | 新特性 | N/A |
| `bugfix/*` | Bug 修复 | N/A |
| `release/*` | 发布准备 | v1.x.0-rc.x |

**PR 工作流**:
```bash
# 1. 新分支
git checkout -b feature/my-feature develop

# 2. 开发、提交、推送
git add . && git commit -m "feat: description"
git push origin feature/my-feature

# 3. GitHub 创建 PR 到 develop

# 4. 审查通过后 merge
```

---

## 📚 文档导航

| 文档 | 用途 |
|------|------|
| [README.md](README.md) | 项目概览、功能、技术栈 |
| [AGENTS.md](AGENTS.md) | AI 代理/开发者快速参考 |
| [MASTER_ROADMAP.md](MASTER_ROADMAP.md) | 完整开发计划、工程规范 |
| [DeviceAutomation_TaskSystem_DesignDoc.md](DeviceAutomation_TaskSystem_DesignDoc.md) | 详细设计文档（中文） |

---

## 🔧 工具链要求

| 工具 | 版本 | 用途 |
|------|------|------|
| GCC / Clang | 13+ / 18+ | 编译 |
| CMake | 3.20+ | 构建系统 |
| C++ | C++20 | 语言标准 |
| SQLite | 3.37+ | 数据持久化 |
| nlohmann/json | 3.11+ | JSON 序列化 |
| Google Test | 1.14+ | 单元测试 |
| Google Benchmark | 1.8+ | 性能测试 |
| clang-format | 14+ | 代码风格 |
| lcov | 2.0+ | 覆盖率报告 |

**检查版本**:
```bash
cmake --version
g++ --version
clang++ --version
sqlite3 --version
```

---

## ❓ FAQ

### Q: 如何添加新的层级或模块？

A: 参考 [MASTER_ROADMAP.md](MASTER_ROADMAP.md) Section 2，遵循分层规则，创建相应的目录和 CMakeLists.txt

### Q: 如何编写单元测试？

A: 参考 [MASTER_ROADMAP.md](MASTER_ROADMAP.md) Section 5.2，使用 Google Test 框架

### Q: 包含规则检查失败怎么办？

A: 运行 `python3 scripts/check_includes.py` 查看具体违反，调整 #include 顺序

### Q: 如何提高代码覆盖率？

A: 在 `tests/unit/` 对应层级添加更多测试用例，运行 `lcov` 生成报告

### Q: 发布流程是什么？

A: 参考 [MASTER_ROADMAP.md](MASTER_ROADMAP.md) Section 8，从 develop 创建 Release PR 到 main

---

## 📞 获取帮助

1. 查看详细文档：[MASTER_ROADMAP.md](MASTER_ROADMAP.md)
2. 检查示例代码（待创建）：`examples/`
3. 查看设计文档：[DeviceAutomation_TaskSystem_DesignDoc.md](DeviceAutomation_TaskSystem_DesignDoc.md)
4. GitHub Issues：提问或报告 bug

---

**最后更新**: May 6, 2026  
**项目阶段**: Design v1.0 + Deployment Planning Complete  
**下一步**: Phase 1 实现开始 (Domain Layer)
