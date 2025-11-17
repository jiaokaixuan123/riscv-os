#include "types.h"
#include "printf.h"
#include "trapframe.h"
#include "proc.h"
#include "riscv.h"
#include "syscall.h"
#include "vm.h"
#include "kalloc.h"
#include "memlayout.h"
#include <string.h>
#include "defs.h" // for panic declaration

// 指令长度检测: 低2位!=0b11 为压缩指令(2字节)，否则4字节
static inline int instr_len(uint64 pc){
  unsigned short first = *(volatile unsigned short*)pc;
  return (first & 0x3) == 0x3 ? 4 : 2;
}

// 统一的内核panic输出助手
static void kernel_panic_with_tf(const char *msg, struct trapframe *tf){
  printf("KERNEL EXCEPTION: %s\n  sepc=%lx stval=%lx scause=%lx sstatus=%lx\n", msg,
         tf?tf->sepc:r_sepc(), r_stval(), r_scause(), tf?tf->sstatus:r_sstatus());
  panic(msg);
}

static uint64 sys_getpid(){ return current ? current->pid : -1; }   // 返回当前进程ID
static uint64 sys_yield(){ yield(); return 0; }                     // 让出CPU
static uint64 sys_exit(){ exit_process(0); return 0; }              // 退出进程  
static uint64 sys_fork(){ return fork_process(); }                  // 创建子进程
static uint64 sys_kill(){ if(!current) return (uint64)-1; return kill_process(current->trapframe->a0); }  // 杀死进程
static uint64 sys_wait(){                                           // 等待子进程退出
  uint64 p = current->trapframe->a0;                                // status pointer
  int status; int pid = wait_process(&status);                      // 等待子进程  
  if(pid > 0 && p){ copyout(current->pagetable, p, &status, sizeof(int)); } // 复制退出码到用户空间
  return pid;
}
extern volatile uint64 ticks;
extern spinlock tickslock;

// 让当前进程睡眠指定的 tick 数
static uint64 sys_sleep(){                                          
  uint64 d = current->trapframe->a0;                                // 获取睡眠时长 
  uint64 target;                                                    // 目标tick数
  acquire(&tickslock);
  target = ticks + d;                                               // 计算目标tick数
  while(ticks < target){
    sleep_chan((void*)&ticks, &tickslock);                          // 睡眠直到ticks变化
  }
  release(&tickslock);
  return 0;
}

// 设置指定进程的优先级
static uint64 sys_setprio(){
  int pid = current->trapframe->a0;
  int pr = current->trapframe->a1;
  return set_priority(pid, pr);
}

// 系统调用处理函数表
static uint64 (*syscalls[])(void) = {
  [SYS_fork] = sys_fork,
  [SYS_exit] = sys_exit,
  [SYS_getpid] = sys_getpid,
  [SYS_yield] = sys_yield,
  [SYS_kill] = sys_kill,
  [SYS_wait] = sys_wait,
  [SYS_sleep] = sys_sleep,
  [SYS_setprio] = sys_setprio,
};

// 处理系统调用
void handle_syscall(void *tfv){
  struct trapframe *tf = (struct trapframe*)tfv;                        // 转换为 trapframe 指针
  uint64 num = tf->a7;                                                  // 系统调用号              
  if(num < sizeof(syscalls)/sizeof(syscalls[0]) && syscalls[num]){      // 有效的系统调用号
    uint64 ret = syscalls[num]();                                       // 调用对应的系统调用处理函数
    tf->a0 = ret;                                                       // 将返回值放入 a0 寄存器         
  } else {
    printf("unknown syscall %lu\n", num);
    tf->a0 = (uint64)-1;
  }
  tf->sepc += instr_len(tf->sepc);                                      // 前进到下一条指令
}

// 检查是否为内核态陷阱
static int is_kernel_trap(struct trapframe *tf)
{ 
  return (tf->sstatus & SSTATUS_SPP) != 0; 
}

// 杀死当前进程，并打印信息
static void kill_current(const char *msg){
  if(current){
    printf("%s: pid=%d sepc=%lx stval=%lx scause=%lx -> kill\n", msg, current->pid, current->trapframe?current->trapframe->sepc:0, r_stval(), r_scause());
    current->killed = 1;
  } else {
    printf("%s (no current) sepc=%lx stval=%lx scause=%lx\n", msg, r_sepc(), r_stval(), r_scause());
  }
}

// 处理指令地址未对齐异常
void handle_instr_addr_misaligned(struct trapframe *tf){
  if(is_kernel_trap(tf)){
    kernel_panic_with_tf("instr addr misaligned", tf);
  } else {
    kill_current("instr addr misaligned");
    // 不前移，被杀进程会退出
  }
}

// 处理指令访问错误异常
void handle_instr_access_fault(struct trapframe *tf){
  if(is_kernel_trap(tf)){
    kernel_panic_with_tf("instr access fault", tf);
  } else {
    kill_current("instr access fault");
  }
}

// 处理非法指令异常
void handle_illegal_instruction(struct trapframe *tf){
  if(is_kernel_trap(tf)){
    kernel_panic_with_tf("illegal instruction", tf);
  } else {
    kill_current("illegal instr");
    tf->sepc += instr_len(tf->sepc); // 跳过非法指令继续
  }
}

// 处理断点异常
void handle_breakpoint(struct trapframe *tf){
  printf("breakpoint sepc=%lx\n", tf->sepc);
  tf->sepc += instr_len(tf->sepc); // 单步继续
}

// 处理加载地址未对齐异常
void handle_load_addr_misaligned(struct trapframe *tf){
  if(is_kernel_trap(tf)) 
    kernel_panic_with_tf("load addr misaligned", tf); 
  else 
    { kill_current("load addr misaligned"); }
}

// 处理加载访问错误异常
void handle_load_access_fault(struct trapframe *tf){
  if(is_kernel_trap(tf)) 
    kernel_panic_with_tf("load access fault", tf); 
  else 
    { kill_current("load access fault"); }
}

// 处理存储地址未对齐异常
void handle_store_addr_misaligned(struct trapframe *tf){
  if(is_kernel_trap(tf)) 
    kernel_panic_with_tf("store addr misaligned", tf); 
  else 
    { kill_current("store addr misaligned"); }
}

// 处理存储访问错误异常
void handle_store_access_fault(struct trapframe *tf){
  if(is_kernel_trap(tf)) 
    kernel_panic_with_tf("store access fault", tf); 
  else 
    { kill_current("store access fault"); }
}

// ================== 页错误懒分配 ==================
static int lazy_alloc_user(uint64 va, int perm){
  if(!current || !current->pagetable) return -1;              // 没有当前进程
  if(va >= USER_MAX_VA) return -1;                            // 超出用户空间
  uint64 aligned = va & ~(PGSIZE-1);                          // 页对齐
  pte_t *pte = walk_lookup(current->pagetable, aligned);      // 查找页表项
  if(pte && (*pte & PTE_V)) return 0;                         // 已映射
  void *page = alloc_page();                                  // 分配物理页
  if(!page) return -1;                                        // 分配失败
  memset(page, 0, PGSIZE);                                    // 清零
  map_region(current->pagetable, aligned, (uint64)page, PGSIZE, perm | PTE_U); // 建立映射
  return 0;
}

void handle_instruction_page_fault(struct trapframe *tf){
  uint64 badva = r_stval();
  if(is_kernel_trap(tf)){
    kernel_panic_with_tf("inst page fault", tf);
    return;
  }
  if(lazy_alloc_user(badva, PTE_R | PTE_X) == 0){
    return; // 重试同一条指令
  }
  kill_current("inst page fault");
}
void handle_load_page_fault(struct trapframe *tf){
  uint64 badva = r_stval();
  if(is_kernel_trap(tf)){
    kernel_panic_with_tf("load page fault", tf);
    return;
  }
  if(lazy_alloc_user(badva, PTE_R) == 0){
    return; // 重试
  }
  kill_current("load page fault");
}
void handle_store_page_fault(struct trapframe *tf){
  uint64 badva = r_stval();
  if(is_kernel_trap(tf)){
    kernel_panic_with_tf("store page fault", tf);
    return;
  }
  if(lazy_alloc_user(badva, PTE_R | PTE_W) == 0){
    return; // 重试
  }
  kill_current("store page fault");
}
