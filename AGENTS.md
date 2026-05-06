# Device Automation Task System — Agent Guidance

> **System Focus**: This is a design-phase project for a sophisticated task management framework for laboratory/industrial device automation. The design document defines comprehensive architecture, components, and mechanisms, but implementation is pending.

## Quick Context

### What is this system?

A general-purpose task management framework that automates lab/industrial device workflows with:
- **High reliability**: Samples are costly; no single task failure causes batch data loss
- **Full traceability**: Every sample from queue entry to completion is auditable
- **Dynamic extensibility**: Add devices, protocols, business processes without core changes
- **Safe intervention**: Pause, resume, rollback at any stage with human oversight
- **Real-time observability**: Monitor system state, task progress, resource utilization

See [DeviceAutomation_TaskSystem_DesignDoc.md](DeviceAutomation_TaskSystem_DesignDoc.md) for complete design details.

### Architecture Overview

The system spans 4 layers:

1. **UI Layer**: Task editor, real-time monitoring, intervention console, audit queries
2. **Application Layer**: WorkflowManager, ManualInterventionService, AuditService
3. **Domain Layer**: Task, TaskGraph, FSM, Checkpoint, RetryPolicy, Event models
4. **Infrastructure Layer**: DeviceAbstraction, PluginLoader, EventBus, Database, WatchDog

## Key Design Principles

### Task Model & FSM

- **Task Types**: Atomic (device operations), Composite (sub-task orchestration), Script (plugin-extensible)
- **Task States**: Pending → Ready → Running → Completed/Failed/Cancelled/RollingBack/RolledBack/WaitingForHuman/Paused
- **State Transitions**: Governed by events (dependencies_met, error, done, pause, resume, retry, rollback, checkpoint, etc.)
- See FSM diagram in `任务 FSM 状态机.png`

### DAG Scheduling Engine

Responsibilities:
- **Dependency parsing**: Detect when prerequisites complete
- **Priority ordering**: Multi-READY tasks sorted by priority + wait duration
- **Resource matching**: Assign tasks to idle, capability-matched executors
- **Flow control**: Prevent task floods (token bucket / leaky bucket)
- **Deadlock detection**: Identify circular dependencies
- **Resource isolation**: Separate executor pools by priority

Pluggable scheduling strategies: RoundRobin, DeviceAffinity, LoadBalance, PriorityFirst

### Executor Layer

Four executor types:
- `ThreadPoolExecutor` — general compute, data processing
- `CoroutineExecutor` — I/O-intensive, device polling
- `DeviceExecutor` — bound to physical device, serial operations
- `ScriptExecutor` — user scripts, sandboxed

### Fault Tolerance & Intervention

- **Retry policies**: Configurable max retries, backoff strategies
- **Checkpoints**: Save state at key points; resume from checkpoint on retry
- **Manual intervention**: Pause/resume, manual retry, retry from checkpoint, human-forced completion (audited)
- **Rollback**: Revert to checkpoint, undo device state (via device-specific cleanup)

### Auditing & Persistence

- Every task state change, retry, intervention is logged with timestamp + operator
- Task state and audit logs persist to disk to survive crashes
- Full workflow history preserved for compliance/debugging

### Extensibility Mechanisms

1. **Plugin System**: User scripts, custom device drivers, custom scheduling strategies
2. **Device Abstraction**: Protocol-agnostic device interface (e.g., MQTT, USB, HTTP APIs)
3. **Event Bus**: Decoupled component communication
4. **Event Types**: Task state change, device event, checkpoint, error, intervention

### Constraints & Priorities

- **Single-machine first**: Local autonomous operation; cloud is auxiliary
- **Device latency <100ms**: Non-hard-realtime but predictable response needed
- **Data persistence**: Mandatory to disk (crash resilience)

## Common Tasks for Agents

### Understanding the Design

**Goal**: Onboard to the design

- Start with [DeviceAutomation_TaskSystem_DesignDoc.md](DeviceAutomation_TaskSystem_DesignDoc.md) sections:
  1. System Goals & Constraints
  2. Overall Architecture
  3. Task Model & FSM
  4. DAG Scheduling Engine
  5. Executor Layer
  6. Fault Tolerance
- Reference the FSM state machine diagram (`任务 FSM 状态机.png`) when designing state transitions

### Implementing a Component

**Goal**: Implement a layer, engine, or service

**Pattern**:
1. Define interfaces/abstractions (e.g., `IExecutor`, `SchedulingPolicy`, `IDevice`)
2. Implement core contract (state transitions, dependency logic, error handling)
3. Integrate audit logging (timestamp, operator, reason)
4. Add persistence hooks (serialize state, restore on restart)
5. Document assumptions (e.g., thread safety, callback ordering, error semantics)

**Key Pitfalls**:
- Forgetting audit logging makes troubleshooting nearly impossible
- Skipping persistence/recovery logic creates data loss on crashes
- Not validating FSM state transitions risks invalid states
- Circular dependencies in task graphs cause deadlock

### Adding a Device Type or Protocol

**Pattern**:
1. Implement `IDevice` abstraction (connect, send, receive, disconnect, cleanup)
2. Define protocol handler (e.g., MQTT client, USB driver)
3. Implement error recovery (timeouts, retries, state reset)
4. Add health checks / watchdog integration
5. Define cleanup/rollback semantics (what happens on failure)
6. Document latency SLA and retry behavior

### Designing Scheduling Strategy

**Pattern**:
1. Implement `SchedulingPolicy` interface
2. Evaluate available executors + task requirements (device affinity, priority, load)
3. Return executor ID
4. Consider: resource starvation, priority inversion, device serialization
5. Log assignment decisions for debugging

### Adding User Intervention Points

**Pattern**:
1. Identify safe states (only RUNNING, PAUSED, FAILED, WAITING_FOR_HUMAN allow intervention)
2. Define intervention action (pause, resume, retry, force_complete, rollback)
3. Validate preconditions (dependencies, state, permissions)
4. Audit: log operator, reason, timestamp, old/new state
5. Update persistent state before notifying subscribers

## Design Trade-offs to Understand

| Trade-off | Decision |
|-----------|----------|
| Consistency vs. Availability | Prioritize consistency + persistence over cluster availability; single-machine strong |
| Retry complexity | Simple exponential backoff sufficient; manual intervention for complex recovery |
| Event ordering | Event bus does not guarantee global ordering; use sequence numbers in events |
| Plugin security | Plugins run in same process; no hard sandbox (design assumes trusted plugins) |
| Device latency | <100ms soft target; watchdog + circuit breaker prevents device hangs from blocking system |

## Documentation References

- **Full Design**: [DeviceAutomation_TaskSystem_DesignDoc.md](DeviceAutomation_TaskSystem_DesignDoc.md)
- **FSM Diagram**: `任务 FSM 状态机.png`
- Sections covering: Task Model, DAG Scheduling, Executors, Fault Tolerance, Auditing, Devices, Plugins, Event Bus, Health Monitoring, Configuration, Security

## Development Notes

- Project is in design phase (v1.0); no implementation code yet
- Likely language: C++ (based on struct syntax in design doc)
- Key implementation priorities:
  1. Task FSM engine (core state machine)
  2. DAG scheduler
  3. Executor pool + coordination
  4. Persistence + recovery
  5. Device abstraction
  6. Plugin system
  7. Audit logging
- Heavy emphasis on reliability and traceability suggests production-grade code expectations

