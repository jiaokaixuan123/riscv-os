#ifndef PROC_H
#define PROC_H
#include "types.h"
#include "lock.h"
#include "vm.h"      // for pagetable_t
#include "trapframe.h" // for struct trapframe

#define NPROC 32

// 进程状态：使用中、就绪、运行、睡眠、僵尸
enum procstate { UNUSED=0, USED, RUNNABLE, RUNNING, SLEEPING, ZOMBIE };

// 调度保存的寄存器上下文 (只保存被调用者保存 + ra + sp)
struct context {
  uint64 ra;    // 返回地址
  uint64 sp;    // 栈指针
  uint64 s0; uint64 s1; uint64 s2; uint64 s3; uint64 s4; uint64 s5;  
  uint64 s6; uint64 s7; uint64 s8; uint64 s9; uint64 s10; uint64 s11; // callee-saved 寄存器
};

// 进程结构体
struct proc {
  spinlock lock;                          // 进程锁
  enum procstate state;                   // 进程状态
  int pid;                                // 进程ID
  int killed;                             // 是否被杀死
  int xstate;                             // 退出码
  struct proc *parent;                    // 父进程
  char name[16];                          // 进程名称
  void *chan;                             // 睡眠通道
  struct context context;                 // 内核线程上下文
  void *kstack;                           // 内核栈底（页基地址）
  uint64 run_ticks;                       // 累计运行 tick 数
  uint64 sched_count;                     // 被调度次数
  uint32 slice_ticks;                     // 当前时间片已用 tick
  // 用户态支持新增
  pagetable_t pagetable;                   // 用户页表 (未来fork复制)
  struct trapframe *trapframe;             // 陷阱帧 (保存用户寄存器)
  int priority;                            // 调度优先级 (数值大优先)
  int base_priority;                       // 基准优先级用于老化恢复
  int is_kernel_thread;                    // 1=内核线程(无用户页表),0=用户进程
};

extern struct proc proc[NPROC];              // 进程表
extern struct proc *current;                 // 当前运行进程指针
extern struct proc *initproc;                // 初始化进程指针

// 接口
void proc_init(void);
struct proc* alloc_process(void);
int create_process(void (*entry)(void), const char *name);
void exit_process(int status);
int wait_process(int *status);
void scheduler(void);
void yield(void);
void wakeup(void *chan);
void sleep_chan(void *chan, spinlock *lk);
void debug_proc_table(void);
static inline const char *proc_state_name(enum procstate s){
  switch(s){
    case UNUSED: return "UNUSED"; case USED: return "USED"; case RUNNABLE: return "RUNNABLE"; case RUNNING: return "RUNNING"; case SLEEPING: return "SLEEPING"; case ZOMBIE: return "ZOMBIE"; default: return "?"; }
}
int fork_process(void); // 类 fork，内核线程简化版
int kill_process(int pid);
int wait_all(void); // 便利测试: 等待所有子进程
int set_priority(int pid, int prio);
int user_init(void);

#endif
