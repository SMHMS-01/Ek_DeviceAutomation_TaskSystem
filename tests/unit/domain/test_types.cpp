#include <gtest/gtest.h>

#include "domain/Types.h"
#include "domain/Priority.h"

using namespace device_automation::domain;

// ============================================================================
// TaskId Tests
// ============================================================================

TEST(TypesTest, TaskIdGenerateCreatesUniqueIds)
{
    auto id1 = TaskId::generate();
    auto id2 = TaskId::generate();
    
    EXPECT_NE(id1, id2);
    EXPECT_FALSE(id1.to_string().empty());
    EXPECT_FALSE(id2.to_string().empty());
}

TEST(TypesTest, TaskIdComparison)
{
    auto id1 = TaskId::generate();
    auto id2 = id1; // 复制
    auto id3 = TaskId::generate();
    
    EXPECT_EQ(id1, id2);
    EXPECT_NE(id1, id3);
}

TEST(TypesTest, TaskIdOrdering)
{
    auto id1 = TaskId("aaa");
    auto id2 = TaskId("bbb");
    
    EXPECT_LT(id1, id2);
    EXPECT_GT(id2, id1);
}

TEST(TypesTest, TaskIdHashable)
{
    std::unordered_set<TaskId> id_set;
    auto id1 = TaskId::generate();
    auto id2 = TaskId::generate();
    
    id_set.insert(id1);
    id_set.insert(id2);
    
    EXPECT_EQ(id_set.size(), 2);
    EXPECT_TRUE(id_set.count(id1) > 0);
    EXPECT_TRUE(id_set.count(id2) > 0);
}

// ============================================================================
// WorkflowId Tests
// ============================================================================

TEST(TypesTest, WorkflowIdGenerateCreatesUniqueIds)
{
    auto wf1 = WorkflowId::generate();
    auto wf2 = WorkflowId::generate();
    
    EXPECT_NE(wf1, wf2);
    EXPECT_FALSE(wf1.to_string().empty());
}

// ============================================================================
// ExecutorId Tests
// ============================================================================

TEST(TypesTest, ExecutorIdFromInt)
{
    auto exec_id = ExecutorId(42);
    EXPECT_EQ(exec_id.to_string(), "42");
}

TEST(TypesTest, ExecutorIdFromString)
{
    auto exec_id = ExecutorId("executor_001");
    EXPECT_EQ(exec_id.to_string(), "executor_001");
}

// ============================================================================
// DeviceId Tests
// ============================================================================

TEST(TypesTest, DeviceIdEmpty)
{
    DeviceId empty_id;
    EXPECT_TRUE(empty_id.is_empty());
    
    DeviceId non_empty_id("device_001");
    EXPECT_FALSE(non_empty_id.is_empty());
}

TEST(TypesTest, DeviceIdComparison)
{
    DeviceId id1("device_1");
    DeviceId id2("device_1");
    DeviceId id3("device_2");
    
    EXPECT_EQ(id1, id2);
    EXPECT_NE(id1, id3);
}

// ============================================================================
// EventId Tests
// ============================================================================

TEST(TypesTest, EventIdGenerateCreatesUniqueIds)
{
    auto event1 = EventId::generate();
    auto event2 = EventId::generate();
    
    EXPECT_NE(event1, event2);
}

// ============================================================================
// Timestamp Tests
// ============================================================================

TEST(TypesTest, TimestampNow)
{
    auto ts1 = Timestamp::now();
    auto ts2 = Timestamp::now();
    
    // 两个时间戳应该非常接近（在 100ms 之内）
    EXPECT_LE(ts2.duration_since(ts1), 100);
}

TEST(TypesTest, TimestampComparison)
{
    auto ts1 = Timestamp(1000);
    auto ts2 = Timestamp(2000);
    auto ts3 = Timestamp(1000);
    
    EXPECT_LT(ts1, ts2);
    EXPECT_LE(ts1, ts2);
    EXPECT_GT(ts2, ts1);
    EXPECT_GE(ts2, ts1);
    EXPECT_EQ(ts1, ts3);
}

TEST(TypesTest, TimestampDurationCalculation)
{
    auto ts1 = Timestamp(1000);
    auto ts2 = Timestamp(3000);
    
    EXPECT_EQ(ts2.duration_since(ts1), 2000);
    EXPECT_EQ(ts1.duration_since(ts2), -2000);
}

TEST(TypesTest, TimestampMillisAndSeconds)
{
    auto ts = Timestamp(5000);
    EXPECT_EQ(ts.millis(), 5000);
    EXPECT_EQ(ts.seconds(), 5);
}

// ============================================================================
// Priority Tests
// ============================================================================

TEST(PriorityTest, PriorityValues)
{
    EXPECT_LT(Priority::Critical, Priority::High);
    EXPECT_LT(Priority::High, Priority::Normal);
    EXPECT_LT(Priority::Normal, Priority::Low);
    EXPECT_LT(Priority::Low, Priority::Background);
}

TEST(PriorityTest, PriorityComparison)
{
    auto critical = Priority::Critical;
    auto high = Priority::High;
    
    EXPECT_LT(critical, high);
    EXPECT_LE(critical, high);
    EXPECT_GT(high, critical);
    EXPECT_GE(high, critical);
}

TEST(PriorityTest, PriorityEquality)
{
    auto normal1 = Priority::Normal;
    auto normal2 = Priority::Normal;
    
    EXPECT_LE(normal1, normal2);
    EXPECT_GE(normal1, normal2);
}

// ============================================================================
// 集成测试
// ============================================================================

TEST(TypesTest, TypesCanBeUsedInContainers)
{
    std::vector<TaskId> task_ids;
    std::unordered_map<WorkflowId, std::vector<TaskId>> workflows;
    
    auto task_id = TaskId::generate();
    auto workflow_id = WorkflowId::generate();
    
    task_ids.push_back(task_id);
    workflows[workflow_id] = task_ids;
    
    EXPECT_EQ(workflows[workflow_id].size(), 1);
    EXPECT_EQ(workflows[workflow_id][0], task_id);
}
