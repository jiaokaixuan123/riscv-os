// trap.c
#include "interrupt.h"
#include "proc.h"

void trapinit();
void kerneltrap();
uint64 get_ticks();
void panic(const char *);
extern volatile uint64 ticks;
void sleep(uint64 target_ticks);

int create_process(void (*entry)(void), const char *name);
void scheduler(void);
void yield(void);
void exit_process(int status);

// 性能测试相关接口与变量声明
void reset_isr_stats(void);
void set_timer_interval(uint64 interval);
void get_isr_stats(uint64 *count, uint64 *cycles);
extern volatile uint64 isr_cycles_sum;
extern volatile uint64 isr_count;
extern volatile uint64 timer_interval;
extern volatile uint64 timer_interrupts;
extern volatile uint64 ctx_switch_cycles_sum;
extern volatile uint64 ctx_switch_count;

// fs.c - 实验7：文件系统
void fsinit(void);
void fileinit(void);
struct inode* iget(uint32 dev, uint32 inum);
void debug_filesystem_state(void);
void debug_inode_usage(void);
void debug_disk_io(void);