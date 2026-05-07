# Device Automation Task System

> A sophisticated, production-grade task management framework for laboratory and industrial device automation.

## Overview

The Device Automation Task System is a comprehensive framework designed to automate complex lab/industrial equipment workflows with exceptional reliability, full traceability, and safe human intervention capabilities. This project prioritizes:

- **High Reliability**: One task failure doesn't cascade to lose entire batch data
- **Full Traceability**: Every sample audited from queue entry to completion
- **Dynamic Extensibility**: Add devices, protocols, and business processes without core modifications
- **Safe Intervention**: Pause, resume, and rollback at any stage with full human oversight
- **Real-time Observability**: Monitor system state, task progress, and resource utilization

## Key Features

### Task Model & FSM
- **Task Types**: Atomic (device operations), Composite (orchestration), Script (plugin-extensible)
- **Rich State Machine**: 10+ states with well-defined transitions (Pending → Ready → Running → Completed/Failed/etc.)
- **Dependency Resolution**: Full DAG (Directed Acyclic Graph) support for complex workflows

### DAG Scheduling Engine
- Intelligent dependency parsing and priority-based scheduling
- Pluggable scheduling strategies (RoundRobin, DeviceAffinity, LoadBalance, PriorityFirst)
- Flow control and deadlock detection
- Resource isolation by priority level

### Execution Layer
- **ThreadPoolExecutor**: General compute and data processing
- **CoroutineExecutor**: I/O-intensive device polling
- **DeviceExecutor**: Physical device binding with serial operations
- **ScriptExecutor**: User-defined scripts with sandboxing

### Fault Tolerance & Recovery
- Configurable retry policies with exponential backoff
- Checkpoint-based state saving for resume-on-retry
- Manual intervention: pause/resume, force-complete, rollback
- Device state cleanup and recovery on failure

### Comprehensive Auditing
- Every task state change logged with operator, timestamp, and reason
- Full workflow history for compliance and debugging
- Persistent storage to survive crashes

### Extensibility
- Plugin system for custom drivers and strategies
- Protocol-agnostic device abstraction (MQTT, USB, HTTP, etc.)
- Event bus for decoupled component communication

## Architecture

```
┌─────────────────────────────────────────────┐
│        UI Layer (Task Editor, Monitoring)   │
├─────────────────────────────────────────────┤
│   Application Layer (Workflow, Intervention)│
├─────────────────────────────────────────────┤
│   Domain Layer (Task, FSM, Event models)    │
├─────────────────────────────────────────────┤
│  Infrastructure (Device, Plugin, EventBus)  │
└─────────────────────────────────────────────┘
```

## Project Status

**Current Phase**: Core scaffold implemented (v1.1)

- ✅ Original architecture and design document
- ✅ Optimized v1.1 design document with acceptance boundary
- ✅ Task FSM engine and base domain types
- ✅ Task and TaskGraph implementation with DAG cycle detection
- ✅ Simple DAG scheduler with SQLite-backed audit writes
- ✅ AuditService with task audit replay and interrupted task recovery
- ✅ Thread-safe EventBus and SQLite infrastructure implementation
- ✅ Standalone and end-to-end acceptance tests

## Documentation

### Core References
- [Design Document](DeviceAutomation_TaskSystem_DesignDoc.md) — Original full technical specification covering:
  - System goals and constraints
  - Complete architecture
  - Task model and FSM
  - DAG scheduling engine
  - Executor layer
  - Fault tolerance and recovery mechanisms
  - Auditing and persistence
  - Device abstraction and communication protocols
  - Plugin system
  - Event bus architecture
  - Health monitoring and watchdog
  - Configuration management
  - Security and permissions

- [Agent Guidance](AGENTS.md) — Quick reference for AI agents and developers:
  - Architecture overview
  - Design principles
  - Common implementation tasks
  - Design trade-offs and considerations

- [FSM State Machine Diagram](任务%20FSM%20状态机.png) — Visual representation of task state transitions

- [Optimized Design v1.1](DeviceAutomation_TaskSystem_DesignDoc_v1.1_Optimized.md) — Engineering refinement covering:
  - v1.0 design gaps
  - minimum acceptance scope
  - implemented module boundaries
  - current verification workflow

## Design Principles

### Core Constraints
- **Single-machine first**: Local autonomous operation with cloud as auxiliary
- **Device latency <100ms**: Non-hard-realtime but predictable response
- **Data persistence**: Mandatory disk storage for crash resilience

### Key Trade-offs
| Area | Decision |
|------|----------|
| Consistency vs. Availability | Prioritize strong consistency + persistence |
| Retry Complexity | Simple exponential backoff; manual intervention for complex recovery |
| Event Ordering | No global ordering guarantee; use sequence numbers |
| Plugin Security | Same-process execution (trusts plugins) |
| Device Control | Watchdog + circuit breaker prevent device hangs |

## Getting Started

### For Design Review
1. Read the [Design Document](DeviceAutomation_TaskSystem_DesignDoc.md) starting with "System Goals & Constraints"
2. Review the [FSM diagram](任务%20FSM%20状态机.png) for state machine overview
3. Check [AGENTS.md](AGENTS.md) for implementation patterns

### Build and Acceptance

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

Current acceptance includes standalone domain/FSM checks, an end-to-end DAG workflow, and persistence recovery verification.

### For Implementation
The project is ready for continued development across the following components:
1. **Executor Pool** — Thread/coroutine/device executors
2. **Recovery Queries** — Database query APIs and restart recovery
3. **Manual Intervention Service** — Permissioned pause/resume/retry/force-complete
4. **WatchDog and TimeoutPolicy** — Soft/hard timeout handling
5. **Device Abstraction** — Protocol-agnostic device interface
6. **Plugin System** — Extensible script and driver loading

## Technical Stack

**Recommended** (based on design document patterns):
- **Language**: C++17+
- **Concurrency**: Thread pool, coroutines, async I/O
- **Serialization**: JSON/YAML for configuration and audit logs
- **Persistence**: SQLite or embedded database for state storage
- **Communication**: Protocol buffers for device abstraction

See Section 14 of the design document for detailed open-source library recommendations.

## Version

- **Version**: 1.1.0-core-scaffold
- **Status**: Core scaffold implemented and acceptance-tested
- **Release Date**: May 2026

## Contributing

Contributions should follow the patterns outlined in [AGENTS.md](AGENTS.md):
- Implement interfaces and abstractions first
- Integrate audit logging into all state-changing operations
- Add persistence hooks for all mutable state
- Document FSM assumptions and thread-safety guarantees
- Validate circular dependency prevention

## License

[To be determined]

## References

- **Design Document** (Chinese): [DeviceAutomation_TaskSystem_DesignDoc.md](DeviceAutomation_TaskSystem_DesignDoc.md)
- **FSM Diagram**: [任务 FSM 状态机.png](任务%20FSM%20状态机.png)
- **Developer Guidance**: [AGENTS.md](AGENTS.md)

---

**Last Updated**: May 6, 2026 | **Phase**: Design v1.0
