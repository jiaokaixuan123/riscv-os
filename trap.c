// 中断和异常处理
#include "types.h"
#include "riscv.h"
#include "lock.h"
#include "printf.h"
#include "interrupt.h"
#include "trapframe.h"
#include "defs.h"
#include "proc.h"
extern struct proc *current;
// 前置原型声明，实际实现位于 exceptions.c
struct trapframe;
// 系统调用与异常处理函数原型补充，避免隐式声明警告
void handle_syscall(void *);                      // 处理系统调用 (匹配 exceptions.c 签名)
void handle_instruction_page_fault(struct trapframe *);       // 处理指令页错误
void handle_load_page_fault(struct trapframe *);              // 处理加载页错误
void handle_store_page_fault(struct trapframe *);             // 处理存储页错误
void handle_instr_addr_misaligned(struct trapframe *);
void handle_instr_access_fault(struct trapframe *);
void handle_illegal_instruction(struct trapframe *);
void handle_breakpoint(struct trapframe *);
void handle_load_addr_misaligned(struct trapframe *);
void handle_load_access_fault(struct trapframe *);
void handle_store_addr_misaligned(struct trapframe *);
void handle_store_access_fault(struct trapframe *);

static void handle_exception_dispatch(void);                  // 异常分发处理
void usertrapret();                                    // 返回用户态 (made global)
void usertrap();                                       // 处理来自用户态的 trap (made global)
extern void userret(struct trapframe *);                      // 返回用户态函数
extern char trampoline[];                                     // 来自 trampoline.S
extern char uservec[];                                        // 用户陷阱入口标签

// ticks 计数与自旋锁
spinlock tickslock; // 全局时钟锁
volatile uint64 ticks; // 全局tick计数，对 sleep 等可见
volatile uint64 timer_interval = 100000;      // 可调定时器间隔 (cycles)
volatile uint64 isr_cycles_sum = 0;            // 中断处理累计周期
volatile uint64 isr_count = 0;                 // 中断次数(与 timer_interrupts 类似,但统计用)
// 新增：暴露上下文切换统计在此文件中也可引用（声明在 proc.c 定义）
extern volatile uint64 ctx_switch_cycles_sum;
extern volatile uint64 ctx_switch_count;

extern void kernelvec();                   // 内核中断入口（汇编实现）                

#define MAX_IRQ 16  // 最大支持的中断号

static interrupt_handler_t irq_table[MAX_IRQ];  // 中断处理函数表
volatile uint64 timer_interrupts;               // 时钟中断次数

static void default_handler(void){ /* 空 */ }   // 默认空处理

// 全局ticks/tickslock保留

// 初始化 trap：设置 stvec 指向 kernelvec
void trap_init(void){
  initlock(&tickslock);
  for(int i=0;i<MAX_IRQ;i++) irq_table[i] = default_handler;  // 默认空处理
  w_stvec((uint64)kernelvec);                                 // 设置中断入口
  // 允许S态中断
  intr_on();  
  // 注册timer irq号采用内部约定 5 (scause bit)
  register_interrupt(5, default_handler); // 可被用户覆盖
}

// 注册中断处理函数
void register_interrupt(int irq, interrupt_handler_t h){
  if(irq >=0 && irq < MAX_IRQ && h) {
    irq_table[irq] = h; 
  }
}

// 启用指定中断号
void enable_interrupt(int irq){ 
  (void)irq; 
  intr_on(); 
}

// 禁用指定中断号
void disable_interrupt(int irq)
{ 
  (void)irq; 
  intr_off(); 
}

// 获取当前时间（ticks）
uint64 get_time(void){ return r_time(); }

// timer专用处理
static void clockintr(){
  uint64 h_start = r_time();                   // 统计开始
  uint64 s = r_sstatus();
  w_sstatus(s & ~SSTATUS_SIE);
  acquire(&tickslock);
  ticks++; timer_interrupts++;
  wakeup((void*)&ticks);
  release(&tickslock);
  if((ticks % 100)==0) printf("[sched hint] ticks=%lu\n", ticks);
  if(current){
    acquire(&current->lock);
    if(current->state == RUNNING){
      current->slice_ticks++; current->run_ticks++;
      if(current->killed){ int pid=current->pid; release(&current->lock); printf("[kill] pid=%d forced exit\n", pid); exit_process(-1); }
      else if(current->slice_ticks >= 5){ current->slice_ticks=0; release(&current->lock); yield(); }
      else release(&current->lock);
    } else release(&current->lock);
  }
  w_stimecmp(r_time() + timer_interval);       // 使用可调间隔
  w_sstatus(s);
  uint64 h_end = r_time();                     // 统计结束
  isr_cycles_sum += (h_end - h_start);
  isr_count++;
}

// 处理中断，返回异常
static int devintr(){
  uint64 sc = r_scause();                                 // 读取 scause 寄存器
  if((sc >> 63) == 0){                                    // 异常而非中断
    handle_exception_dispatch();                          // 处理异常
    return 1;                                             // 异常不视为设备中断
  }
  if(sc == 0x8000000000000005L){                          // 如果是特权级的时钟中断
    clockintr();                                          // 处理时钟中断
    return 2;
  }
  return 0; // 未识别
}

// 处理来自内核态的 trap（目前仅 timer）
void kerneltrap() {
  uint64 sc = r_scause();
  uint64 epc = r_sepc();                            // 保存异常返回地址
  if((r_sstatus() & SSTATUS_SPP) == 0){             // 如果是从用户态进入内核态
    usertrap();                                     // 重定向到用户态陷阱处理
    return;
  }
  int which = devintr();
  if(which == 0){
    printf("kerneltrap: unhandled scause=%lx sepc=%lx\n", sc, epc);
  } else if(which == 2){
    // 中断：不前进 PC，保持原值
    w_sepc(epc);
  }
  // which == 1 (异常) 时不写回 epc，保留处理函数调整后的 sepc
}

// 暴露一个获取 ticks 的函数供测试代码使用
uint64 get_ticks() { return ticks; }

// 新增：重置 ISR 统计
void reset_isr_stats(void){
  isr_cycles_sum = 0;
  isr_count = 0;
}
// 新增：设置新的定时器间隔（最小阈值避免过于频繁）
void set_timer_interval(uint64 interval){
  if(interval < 1000) interval = 1000; // 简单下限防止过载
  timer_interval = interval;
  // 立即重置下一次比较值
  w_stimecmp(r_time() + timer_interval);
}
// 可选：获取 ISR 统计（供后续需要）
void get_isr_stats(uint64 *count, uint64 *cycles){
  if(count) *count = isr_count;
  if(cycles) *cycles = isr_cycles_sum;
}

// 异常分发处理
static void handle_exception_dispatch(void){
  struct trapframe tf;        
  tf.sepc = r_sepc();
  tf.scause = r_scause();
  tf.sstatus = r_sstatus();
  uint64 cause = tf.scause & 0xfff;
  switch(cause){
    case 0: handle_instr_addr_misaligned(&tf); break;
    case 1: handle_instr_access_fault(&tf); break;
    case 2: handle_illegal_instruction(&tf); break;
    case 3: handle_breakpoint(&tf); break;
    case 4: handle_load_addr_misaligned(&tf); break;
    case 5: handle_load_access_fault(&tf); break;
    case 6: handle_store_addr_misaligned(&tf); break;
    case 7: handle_store_access_fault(&tf); break;
    case 8: // U-mode ecall
      handle_syscall(current ? (void*)current->trapframe : 0);
      return;
    case 9: // S-mode ecall
      printf("env call from S mode\n");
      tf.sepc += 4; break;
    case 12: handle_instruction_page_fault(&tf); break;
    case 13: handle_load_page_fault(&tf); break;
    case 15: handle_store_page_fault(&tf); break;
    default:
      printf("unknown exception cause=%lx stval=%lx\n", cause, r_stval());
      tf.sepc += 4; break;
  }
  w_sepc(tf.sepc);
}

// 返回用户态
void usertrapret(){
  // 设置 stvec 指向 uservec (trampoline 中的用户陷阱入口)
  uint64 trampoline_uservec = (uint64)trampoline + ((uint64)uservec - (uint64)trampoline);
  w_stvec(trampoline_uservec);
  // 配置 sstatus: SPP=0 SPIE=1
  uint64 s = r_sstatus();
  s &= ~SSTATUS_SPP; // 返回用户态
  s |= SSTATUS_SPIE; // 允许用户态中断
  w_sstatus(s);
  w_sepc(current->trapframe->sepc); // 设置返回用户PC
  // 准备 a0 = 用户页表 satp 供 userret 使用
  uint64 user_satp = MAKE_SATP(current->pagetable);
  asm volatile("mv a0, %0" :: "r"(user_satp));
  userret(current->trapframe);
}

// 处理来自用户态的 trap
void usertrap(){
  if((r_sstatus() & SSTATUS_SPP) != 0){                   // 如果是从用户态进入内核态   
    panic("usertrap: not user mode");
  }
  if(!current || !current->trapframe){                    // 如果没有当前进程或陷阱帧
    panic("usertrap: no current process");
  }
  current->trapframe->sepc = r_sepc();                    // 保存异常返回地址
  current->trapframe->sstatus = r_sstatus();              // 保存状态寄存器
  current->trapframe->scause = r_scause();                // 保存异常原因  
  uint64 cause = current->trapframe->scause & 0xfff;
  if(cause == 8){                                         // syscall
    handle_syscall(current->trapframe);
  } else if((current->trapframe->scause >> 63) == 1){     // interrupt
    if(devintr()==0){ /* unknown */ }
  } else {
    printf("usertrap: unexpected scause=%lx pid=%d\n", current->trapframe->scause, current->pid);
    current->killed = 1;                                  // 如果是未知原因的中断，杀死该进程
  }
  if(current->killed){
    exit_process(-1);
    return;
  }
  usertrapret();
}
