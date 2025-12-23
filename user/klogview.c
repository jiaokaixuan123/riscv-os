// 用户空间的内核日志查看器

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/klog.h"
#include "user/user.h"

// 显示用法信息并退出
void
usage(void)
{
    printf("Usage: klogview [options]\n");
    printf("Options:\n");
    printf("  -c          Clear the log buffer\n");
    printf("  -l LEVEL    Show only logs at or above LEVEL\n");
    printf("              Levels: DEBUG(0), INFO(1), WARN(2), ERROR(3), FATAL(4)\n");
    printf("  -s SUBSYS   Show only logs from SUBSYS\n");
    printf("  -n COUNT    Show only the last COUNT entries\n");
    printf("  -t          Show statistics\n");
    printf("  -L LEVEL    Set kernel log level\n");
    printf("  -e          Enable kernel logging\n");
    printf("  -d          Disable kernel logging\n");
    printf("  -h          Show this help\n");
    exit(0);
}

// 解析日志级别字符串
int
parse_level(char *str)
{
    if(strcmp(str, "DEBUG") == 0 || strcmp(str, "0") == 0)
        return LOG_LEVEL_DEBUG;
    if(strcmp(str, "INFO") == 0 || strcmp(str, "1") == 0)
        return LOG_LEVEL_INFO;
    if(strcmp(str, "WARN") == 0 || strcmp(str, "2") == 0)
        return LOG_LEVEL_WARN;
    if(strcmp(str, "ERROR") == 0 || strcmp(str, "3") == 0)
        return LOG_LEVEL_ERROR;
    if(strcmp(str, "FATAL") == 0 || strcmp(str, "4") == 0)
        return LOG_LEVEL_FATAL;
    return -1;
}

// 将日志级别转换为字符串
const char*
level_to_string(int level)
{
    switch(level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO:  return "INFO ";
        case LOG_LEVEL_WARN:  return "WARN ";
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_FATAL: return "FATAL";
        default:              return "UNKN ";
    }
}

// 打印单个日志条目
void
print_entry(struct log_entry *entry)
{
    printf("[%ld] [%d:%d] [%s] [%s] %s:%d: %s\n",
           entry->timestamp,
           entry->cpu_id,
           entry->pid,
           level_to_string(entry->level),
           entry->subsystem,
           entry->file,
           entry->line,
           entry->message);
}

// 显示统计信息
void
show_stats(void)
{
    struct log_stats stats;
    
    if(klogctl(KLOG_CMD_GET_STATS, &stats, 0) < 0) {
        printf("Failed to get statistics\n");
        return;
    }
    
    printf("=== Kernel Log Statistics ===\n");
    printf("Total logs written: %ld\n", stats.total_logs);
    printf("Dropped logs:       %ld\n", stats.dropped_logs);
    printf("Buffer wraps:       %ld\n", stats.buffer_wraps);
    printf("\nLogs by level:\n");
    printf("  DEBUG: %ld\n", stats.logs_by_level[0]);
    printf("  INFO:  %ld\n", stats.logs_by_level[1]);
    printf("  WARN:  %ld\n", stats.logs_by_level[2]);
    printf("  ERROR: %ld\n", stats.logs_by_level[3]);
    printf("  FATAL: %ld\n", stats.logs_by_level[4]);
}

int
main(int argc, char *argv[])
{
    int clear_log = 0;
    int show_last_n = -1;
    int min_level = LOG_LEVEL_DEBUG;
    char *filter_subsys = 0;
    int show_statistics = 0;
    int set_level = -1;
    int enable_log = 0;
    int disable_log = 0;
    
    // 解析命令行参数
    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-c") == 0) {
            clear_log = 1;
        } else if(strcmp(argv[i], "-l") == 0) {
            if(i + 1 >= argc)
                usage();
            min_level = parse_level(argv[++i]);
            if(min_level < 0) {
                printf("Invalid log level: %s\n", argv[i]);
                exit(1);
            }
        } else if(strcmp(argv[i], "-s") == 0) {
            if(i + 1 >= argc)
                usage();
            filter_subsys = argv[++i];
        } else if(strcmp(argv[i], "-n") == 0) {
            if(i + 1 >= argc)
                usage();
            show_last_n = atoi(argv[++i]);
        } else if(strcmp(argv[i], "-t") == 0) {
            show_statistics = 1;
        } else if(strcmp(argv[i], "-L") == 0) {
            if(i + 1 >= argc)
                usage();
            set_level = parse_level(argv[++i]);
            if(set_level < 0) {
                printf("Invalid log level: %s\n", argv[i]);
                exit(1);
            }
        } else if(strcmp(argv[i], "-e") == 0) {
            enable_log = 1;
        } else if(strcmp(argv[i], "-d") == 0) {
            disable_log = 1;
        } else if(strcmp(argv[i], "-h") == 0) {
            usage();
        } else {
            printf("Unknown option: %s\n", argv[i]);
            usage();
        }
    }
    
    // 执行命令
    
    // 设置日志级别
    if(set_level >= 0) {
        if(klogctl(KLOG_CMD_SET_LEVEL, 0, set_level) < 0) {
            printf("Failed to set log level\n");
            exit(1);
        }
        printf("Kernel log level set to %s\n", level_to_string(set_level));
    }
    
    // 启用日志记录
    if(enable_log) {
        if(klogctl(KLOG_CMD_ENABLE, 0, 0) < 0) {
            printf("Failed to enable logging\n");
            exit(1);
        }
        printf("Kernel logging enabled\n");
    }
    
    if(disable_log) {
        if(klogctl(KLOG_CMD_DISABLE, 0, 0) < 0) {
            printf("Failed to disable logging\n");
            exit(1);
        }
        printf("Kernel logging disabled\n");
    }
    
    // 显示统计信息
    if(show_statistics) {
        show_stats();
        if(clear_log || show_last_n >= 0)
            printf("\n");
    }
    
    // 清理日志缓冲区
    if(clear_log) {
        if(klogctl(KLOG_CMD_CLEAR, 0, 0) < 0) {
            printf("Failed to clear log\n");
            exit(1);
        }
        printf("Kernel log cleared\n");
        if(show_last_n < 0)
            exit(0);
    }
    
    // 读取并显示日志条目
    if(!clear_log || show_last_n >= 0) {
        // Get log count
        int count = klogctl(KLOG_CMD_GET_COUNT, 0, 0);
        if(count < 0) {
            printf("Failed to get log count\n");
            exit(1);
        }
        
        if(count == 0) {
            printf("No kernel logs available\n");
            exit(0);
        }
        
        // 分配缓冲区以存储日志条目，避免栈溢出
        int max_read = 256;  // 一次读取最多256条日志
        if(count < max_read)
            max_read = count;
        
        struct log_entry *entries = (struct log_entry*) malloc(sizeof(struct log_entry) * max_read);
        if(entries == 0) {
            printf("Failed to allocate memory for log entries\n");
            exit(1);
        }
        
        // 读取日志
        int read_count = klogctl(KLOG_CMD_READ, entries, max_read);
        if(read_count < 0) {
            printf("Failed to read logs\n");
            free(entries);
            exit(1);
        }
        
        printf("=== Kernel Log (%d entries) ===\n", count);
        
        // 计算起始索引
        int start_idx = 0;
        if(show_last_n >= 0 && show_last_n < read_count)
            start_idx = read_count - show_last_n;

        // 计算实际要显示的条目数
        int shown = 0;
        for(int i = start_idx; i < read_count; i++) {
            // 分析过滤条件
            if(entries[i].level < min_level)
                continue;
            if(filter_subsys && strcmp(entries[i].subsystem, filter_subsys) != 0)
                continue;
            
            print_entry(&entries[i]);
            shown++;
        }
        
        if(shown == 0) {
            printf("(No logs match the filter criteria)\n");
        }
        
        printf("=== End of Log (%d shown) ===\n", shown);
        
        // 释放分配缓冲区
        free(entries);
    }
    
    exit(0);
}
