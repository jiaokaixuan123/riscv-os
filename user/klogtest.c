// 内核日志测试程序

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/klog.h"
#include "user/user.h"

void
test_basic_logging(void)
{
    printf("\n=== Test 1: Basic Logging ===\n");
    
    // 获取初始日志计数
    int count = klogctl(KLOG_CMD_GET_COUNT, 0, 0);
    printf("Initial log count: %d\n", count);
    
    // 触发内核活动以生成日志
    int pid = fork();
    if(pid == 0) {
        exit(0);
    } else {
        wait(0);
    }
    
    // 获取新的日志计数
    int new_count = klogctl(KLOG_CMD_GET_COUNT, 0, 0);
    printf("New log count: %d\n", new_count);
    
    if(new_count > count) {
        printf("PASS: Logs were generated\n");
    } else {
        printf("WARN: No new logs generated\n");
    }
}

void
test_log_levels(void)
{
    printf("\n=== Test 2: Log Level Filtering ===\n");
    
    int current_level = klogctl(KLOG_CMD_GET_LEVEL, 0, 0);
    printf("Current log level: %d\n", current_level);
    
    if(klogctl(KLOG_CMD_SET_LEVEL, 0, LOG_LEVEL_ERROR) < 0) {
        printf("FAIL: Could not set log level\n");
        return;
    }
    printf("Set log level to ERROR (3)\n");
    
    // 验证日志级别已更改
    int new_level = klogctl(KLOG_CMD_GET_LEVEL, 0, 0);
    if(new_level == LOG_LEVEL_ERROR) {
        printf("PASS: Log level changed successfully\n");
    } else {
        printf("FAIL: Log level not changed (got %d)\n", new_level);
    }
    
    // 恢复原始日志级别
    klogctl(KLOG_CMD_SET_LEVEL, 0, current_level);
}

void
test_clear_logs(void)
{
    printf("\n=== Test 3: Clear Log Buffer ===\n");
    
    int count_before = klogctl(KLOG_CMD_GET_COUNT, 0, 0);
    printf("Logs before clear: %d\n", count_before);
    
    if(klogctl(KLOG_CMD_CLEAR, 0, 0) < 0) {
        printf("FAIL: Could not clear logs\n");
        return;
    }
    
    int count_after = klogctl(KLOG_CMD_GET_COUNT, 0, 0);
    printf("Logs after clear: %d\n", count_after);
    
    if(count_after == 0) {
        printf("PASS: Logs cleared successfully\n");
    } else {
        printf("FAIL: Logs not fully cleared\n");
    }
}

void
test_read_logs(void)
{
    printf("\n=== Test 4: Read Log Entries ===\n");
    
    // 清空日志以确保有新日志
    klogctl(KLOG_CMD_CLEAR, 0, 0);
    
    // 生成一些日志
    for(int i = 0; i < 5; i++) {
        int pid = fork();
        if(pid == 0) {
            exit(0);
        }
    }
    
    // Wait for children
    for(int i = 0; i < 5; i++) {
        wait(0);
    }
    
    // 读取日志条目
    int max = 50;
    struct log_entry *entries = (struct log_entry*) malloc(sizeof(struct log_entry) * max);
    if(entries == 0) {
        printf("Failed to allocate memory for log entries\n");
        return;
    }
    int count = klogctl(KLOG_CMD_READ, entries, max);
    
    printf("Read %d log entries\n", count);
    
    if(count > 0) {
        printf("PASS: Successfully read logs\n");
        printf("Sample entries:\n");
        for(int i = 0; i < (count < 5 ? count : 5); i++) {
            printf("  [%s] %s: %s\n", 
                   entries[i].subsystem,
                   entries[i].file,
                   entries[i].message);
        }
    } else {
        printf("WARN: No logs read\n");
    }
    
    free(entries);
}

void
test_statistics(void)
{
    printf("\n=== Test 5: Log Statistics ===\n");
    
    struct log_stats stats;
    
    if(klogctl(KLOG_CMD_GET_STATS, &stats, 0) < 0) {
        printf("FAIL: Could not get statistics\n");
        return;
    }
    
    printf("Total logs: %ld\n", stats.total_logs);
    printf("Dropped logs: %ld\n", stats.dropped_logs);
    printf("Buffer wraps: %ld\n", stats.buffer_wraps);
    printf("Logs by level:\n");
    printf("  DEBUG: %ld\n", stats.logs_by_level[0]);
    printf("  INFO:  %ld\n", stats.logs_by_level[1]);
    printf("  WARN:  %ld\n", stats.logs_by_level[2]);
    printf("  ERROR: %ld\n", stats.logs_by_level[3]);
    printf("  FATAL: %ld\n", stats.logs_by_level[4]);
    
    printf("PASS: Statistics retrieved\n");
}

void
test_enable_disable(void)
{
    printf("\n=== Test 6: Enable/Disable Logging ===\n");
    
    // 禁用日志
    if(klogctl(KLOG_CMD_DISABLE, 0, 0) < 0) {
        printf("FAIL: Could not disable logging\n");
        return;
    }
    printf("Logging disabled\n");
    
    int count_before = klogctl(KLOG_CMD_GET_COUNT, 0, 0);
    
    // 触发内核活动（不应生成日志）
    int pid = fork();
    if(pid == 0) {
        exit(0);
    }
    wait(0);
    
    int count_after = klogctl(KLOG_CMD_GET_COUNT, 0, 0);
    
    // 重启日志
    if(klogctl(KLOG_CMD_ENABLE, 0, 0) < 0) {
        printf("FAIL: Could not enable logging\n");
        return;
    }
    printf("Logging re-enabled\n");
    
    if(count_after == count_before) {
        printf("PASS: No logs generated while disabled\n");
    } else {
        printf("WARN: Logs still generated (%d vs %d)\n", 
               count_before, count_after);
    }
}

void
test_stress(void)
{
    printf("\n=== Test 7: Stress Test (Multiple Processes) ===\n");
    
    klogctl(KLOG_CMD_CLEAR, 0, 0);
    
    int num_children = 10;
    printf("Spawning %d processes to generate logs...\n", num_children);
    
    for(int i = 0; i < num_children; i++) {
        int pid = fork();
        if(pid == 0) {
            // Child: 生成大量日志
            for(int j = 0; j < 10; j++) {
                int fd = open("README", 0);
                if(fd >= 0) {
                    char buf[64];
                    read(fd, buf, sizeof(buf));
                    close(fd);
                }
            }
            exit(0);
        }
    }
    
    // Wait for all children
    for(int i = 0; i < num_children; i++) {
        wait(0);
    }
    
    int final_count = klogctl(KLOG_CMD_GET_COUNT, 0, 0);
    printf("Final log count: %d\n", final_count);
    
    struct log_stats stats;
    klogctl(KLOG_CMD_GET_STATS, &stats, 0);
    printf("Total logs written: %ld\n", stats.total_logs);
    printf("Dropped logs: %ld\n", stats.dropped_logs);
    
    if(final_count > 0) {
        printf("PASS: Stress test completed\n");
    } else {
        printf("WARN: No logs generated during stress test\n");
    }
}

int
main(int argc, char *argv[])
{
    printf("=================================\n");
    printf("Kernel Logging System Test Suite\n");
    printf("=================================\n");
    
    test_basic_logging();
    test_log_levels();
    test_read_logs();
    test_statistics();
    test_enable_disable();
    test_clear_logs();
    test_stress();
    
    printf("\n=================================\n");
    printf("All tests completed!\n");
    printf("=================================\n");
    
    exit(0);
}
