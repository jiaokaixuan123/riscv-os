#include "types.h"
#include "proc.h"
#include "kalloc.h"
#include "string.h"
#include "printf.h"
#include "defs.h"
#include "riscv.h"
#include "lock.h"
#include "vm.h"
#include "trapframe.h"
#include "memlayout.h"
#include <string.h>
#include <stdarg.h>

extern void usertrap();
extern void userret(struct trapframe*);

struct proc proc[NPROC];    // 进程表
struct proc *current = 0;   // 当前运行进程指针
// 新增：上下文切换统计
volatile uint64 ctx_switch_cycles_sum = 0;
volatile uint64 ctx_switch_count = 0;

struct proc *initproc = 0;  // 初始化进程指针
static int nextpid = 1;     // 下一个进程ID
static spinlock pidlock;    // 进程ID锁

static struct context sched_ctx;  // 统一的调度器上下文

extern volatile uint64 ticks;     // 全局tick计数

// 内联读取 tp
static inline uint64 read_tp(){ uint64 x; asm volatile("mv %0, tp" : "=r"(x)); return x; }
// 内联读取 sp 用于调试堆栈使用
static inline uint64 read_sp(){ uint64 x; asm volatile("mv %0, sp" : "=r"(x)); return x; }

// 初始化进程上下文，使其在切换时从 fn 开始执行
static void init_context(struct context *c, void (*fn)(void)){
  memset(c, 0, sizeof(*c)); // 清零上下文
  c->ra = (uint64)fn;       // 返回地址设置为线程体
}

// 映射用户代码到用户页表
static void map_user_code(pagetable_t pt, uint8 *code, uint64 sz);
//
void dump_runnable_counts(void);

// 调试: 简单堆栈窗口转储（低端16字双字）
static void dump_stack_window(void *base){
  uint64 *p = (uint64*)base;
  dprint("[stackdump base=%p] ", base);
  for(int i=0;i<16;i++){ dprint("%lx ", p[i]); }
  dprint("\n");
}

// 内核线程统一包装入口，防止线程函数返回后再次递归执行
static void kernel_thread_start(void){
  dprint("[kernel_thread_start] current=%d sp=%p kstack=%p top=%p\n", current?current->pid:-1, read_sp(), current?current->kstack:0, current?(void*)((uint64)current->kstack+PGSIZE):0);
  if(!current) panic("no current in kernel_thread_start");
  void (*fn)(void) = (void(*)())current->context.s0; // 保存的原始入口
  dprint("[kernel_thread_start] entry=%p name=%s\n", fn, current?current->name:"?");
  fn();
  exit_process(0); // 不返回
  for(;;) yield(); // 保险：绝不返回
}

// 初始化进程表
void proc_init(void){
  initlock(&pidlock);
  for(int i=0;i<NPROC;i++){
    initlock(&proc[i].lock);
    proc[i].state = UNUSED;   // 初始状态为 UNUSED
  }
  memset(&sched_ctx, 0, sizeof(sched_ctx)); // 清零调度器上下文
  // 创建一个 init 进程占位符（内核线程），以便孤儿进程可以重新分配父进程
  dprint("[proc_init] creating init process...\n");
  initproc = alloc_process(); 
  if(initproc){
    acquire(&initproc->lock);
    strncpy(initproc->name, "init", sizeof(initproc->name)-1);  // 设置进程名称
    initproc->name[sizeof(initproc->name)-1] = '\0';            // 确保字符串终止
    initproc->kstack = alloc_page();                            // 分配内核栈
    if(!initproc->kstack){
      release(&initproc->lock);
      panic("proc_init: failed to allocate kstack for initproc");
    }
    init_context(&initproc->context, 0);                        // 初始化上下文
    initproc->state = USED;                                     // 状态为 USED
    release(&initproc->lock);
    dprint("[proc_init] init process created successfully. pid=%d\n", initproc->pid);
    current = initproc; // Set current process to initproc
  } else {
    panic("proc_init: failed to allocate initproc");
  }
}

// 分配唯一的进程ID
static int allocpid(){
  acquire(&pidlock);
  int pid = nextpid++;
  release(&pidlock);
  return pid;
}

// 分配一个新的进程结构体
struct proc* alloc_process(void){
  dprint("[alloc_process] begin scan NPROC=%d current=%d\n", NPROC, current?current->pid:-1);
  for(int i=0;i<NPROC;i++){
    dprint("[alloc_process] check slot=%d\n", i);
    acquire(&proc[i].lock);
    if(proc[i].state == UNUSED){
      proc[i].state = USED;
      proc[i].pid = allocpid();
      proc[i].killed = 0;
      proc[i].xstate = 0;
      proc[i].parent = 0;
      proc[i].chan = 0;             // 
      proc[i].run_ticks = 0;        // 运行时间
      proc[i].sched_count = 0;      
      proc[i].slice_ticks = 0;      
      proc[i].priority = 10;        // 默认优先级
      proc[i].base_priority = 10;   // 基准优先级用于老化恢复
      // 新增: 规范初始化避免旧值残留
      proc[i].kstack = 0;
      proc[i].trapframe = 0;
      proc[i].pagetable = 0;
      proc[i].is_kernel_thread = 0;
      release(&proc[i].lock);
      dprint("[alloc] success pid=%d slot=%d\n", proc[i].pid, i);
      return &proc[i];
    }
    release(&proc[i].lock);
  }
  dprint("[alloc] no free proc slot, nextpid=%d\n", nextpid);
  dump_runnable_counts();
  return 0;
}

// 创建一个新的进程（内核线程）
int create_process(void (*entry)(void), const char *name){
  dprint("[create_process] attempting to create process '%s' (caller pid=%d sp=%p) ...\n", name, current?current->pid:-1, read_sp());
  if(!entry){ dprint("[create] null entry\n"); return -1; }
  struct proc *p = alloc_process(); 
  dprint("[create_process] after alloc_process ptr=%p caller_sp=%p\n", p, read_sp());
  if(!p){ dprint("[create] alloc_process failed for '%s'\n", name); return -1; }
  dprint("[create_process] alloc_process succeeded for '%s', pid=%d\n", name, p->pid);
  acquire(&p->lock);
  dprint("[create_process] acquired lock pid=%d sp=%p\n", p->pid, read_sp());
  p->parent = current;
  p->kstack = alloc_page();
  dprint("[create_process] alloc_page returned %p sp=%p\n", p->kstack, read_sp());
  if(!p->kstack){ 
    p->state=UNUSED; 
    release(&p->lock); 
    dprint("[create] kstack alloc failed for pid=%d name='%s'\n", p->pid, name); 
    return -1; 
  }
  // 预填充栈低端哨兵用于溢出检测
  memset(p->kstack, 0xAA, 128);
  dprint("[create_process] kstack allocated for pid=%d at %p (top=%p)\n", p->pid, p->kstack, (void*)((uint64)p->kstack+PGSIZE));
  p->trapframe = 0;
  p->pagetable = 0;
  p->is_kernel_thread = 1;
  init_context(&p->context, kernel_thread_start);
  p->context.s0 = (uint64)entry;
  p->context.sp = (uint64)p->kstack + PGSIZE;
  strncpy(p->name, name?name:"kth", sizeof(p->name)-1); p->name[sizeof(p->name)-1]='\0';
  p->state = RUNNABLE;
  dprint("[create] pid=%d name=%s entry=%p sp(set)=%p parent=%d caller_sp_now=%p\n", p->pid, p->name, (void*)entry, (void*)p->context.sp, p->parent?p->parent->pid:-1, read_sp());
  // 输出哨兵区域
  dump_stack_window(p->kstack);
  release(&p->lock);
  dprint("[create_process] released lock pid=%d sp=%p\n", p->pid, read_sp());
  dump_runnable_counts();
  return p->pid;
}

// 显式创建用户进程，加载代码和数据
// 仅供 exec 使用
int create_process_ex(void (*entry)(void), const char *name, uint8 *code, uint64 sz){
  (void)entry; // 未使用参数
  struct proc *p = alloc_process();                           
  if(!p) return -1;
  acquire(&p->lock);
  p->parent = current;          // 设置父进程为当前进程
  p->kstack = alloc_page();     // 分配内核栈
  if(!p->kstack){ p->state = UNUSED; release(&p->lock); return -1; }  // 分配失败，状态置为 UNUSED
  p->trapframe = (struct trapframe*)alloc_page();   // 分配trapframe
  if(!p->trapframe)                                 // 分配失败，释放内核栈
  { 
    free_page(p->kstack); 
    p->kstack=0; p->state=UNUSED; 
    release(&p->lock); 
    return -1; 
  }  
  p->pagetable = create_pagetable();                // 创建用户页表
  if(!p->pagetable)                                 // 分配失败，释放trapframe与内核栈
  { 
    free_page(p->trapframe); 
    p->trapframe=0; 
    free_page(p->kstack); 
    p->kstack=0; 
    p->state=UNUSED; 
    release(&p->lock);
    return -1; 
  }
  // 映射用户代码和栈到页表
  map_user_code(p->pagetable, code, sz);
  // 映射 TRAMPOLINE & TRAPFRAME
  extern char trampoline[];  // 来自 trampoline.S
  map_region(p->pagetable, TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X | PTE_U);
  map_region(p->pagetable, TRAPFRAME, (uint64)p->trapframe, PGSIZE, PTE_R | PTE_W | PTE_U);
  // 初始化 trapframe 核心字段
  memset(p->trapframe, 0, sizeof(*p->trapframe));
  p->trapframe->sepc = USER_TEXT_START;                  // 用户初始PC
  p->trapframe->kernel_satp = r_satp();                  // 当前内核页表
  p->trapframe->kernel_sp = (uint64)p->kstack + PGSIZE;  // 内核栈顶
  p->trapframe->kernel_trap = (uint64)usertrap;          // usertrap入口
  p->trapframe->kernel_hartid = read_tp();                  // 保存tp
  p->trapframe->sstatus = r_sstatus() & ~SSTATUS_SPP;    // 准备返回用户态
  extern void usertrapret();
  init_context(&p->context, (void*)usertrapret);
  p->context.sp = (uint64)p->kstack + PGSIZE;
  strncpy(p->name, name?name:"proc", sizeof(p->name)-1);  // 设置进程名称
  p->name[sizeof(p->name)-1] = '\0';
  p->state = RUNNABLE;
  release(&p->lock);
  return p->pid;
}

// 映射用户代码和栈到页表 (definition)
static void map_user_code(pagetable_t pt, uint8 *code, uint64 sz){
  // 映射代码区域
  map_region(pt, USER_TEXT_START, (uint64)code, sz, PTE_R | PTE_X | PTE_U); // 代码区权限：可读、可执行、用户态
  // 映射栈 (分配一页)
  void *stack = alloc_page();
  if(stack) map_region(pt, USER_STACK_TOP - USER_STACK_SIZE, (uint64)stack, USER_STACK_SIZE, PTE_R | PTE_W | PTE_U); // 栈区权限：可读、可写、用户态
}

// 创建初始用户进程
int user_init(void){
  // 简单嵌入式用户代码：无限循环触发环境调用(占位)
  static uint8 ucode[32] = {0x73,0,0,0}; // ecall 循环
  struct proc *p = alloc_process();
  if(!p) return -1;
  acquire(&p->lock);
  p->parent = initproc;
  p->kstack = alloc_page();
  if(!p->kstack){ p->state=UNUSED; release(&p->lock); return -1; }
  p->trapframe = (struct trapframe*)alloc_page();
  if(!p->trapframe){ free_page(p->kstack); p->kstack=0; p->state=UNUSED; release(&p->lock); return -1; }
  p->pagetable = create_pagetable();
  if(!p->pagetable){ free_page(p->trapframe); p->trapframe=0; free_page(p->kstack); p->kstack=0; p->state=UNUSED; release(&p->lock); return -1; }
  // 映射用户代码/栈
  map_user_code(p->pagetable, ucode, sizeof(ucode));
  // 映射 TRAMPOLINE & TRAPFRAME
  extern char trampoline[];  // 来自 trampoline.S
  map_region(p->pagetable, TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X | PTE_U);
  map_region(p->pagetable, TRAPFRAME, (uint64)p->trapframe, PGSIZE, PTE_R | PTE_W | PTE_U);
  // 初始化 trapframe 核心字段
  memset(p->trapframe, 0, sizeof(*p->trapframe));
  p->trapframe->sepc = USER_TEXT_START;                  // 用户初始PC
  p->trapframe->kernel_satp = r_satp();                  // 当前内核页表
  p->trapframe->kernel_sp = (uint64)p->kstack + PGSIZE;  // 内核栈顶
  p->trapframe->kernel_trap = (uint64)usertrap;          // usertrap入口
  p->trapframe->kernel_hartid = read_tp();                  // 保存tp
  p->trapframe->sstatus = r_sstatus() & ~SSTATUS_SPP;    // 准备返回用户态
  // 设置 context 使调度后进入 usertrapret
  extern void usertrapret();
  init_context(&p->context, (void*)usertrapret);
  p->context.sp = (uint64)p->kstack + PGSIZE;
  strncpy(p->name, "userinit", sizeof(p->name)-1);    // 设置进程名称
  p->name[sizeof(p->name)-1] = '\0';
  p->state = RUNNABLE;
  release(&p->lock);
  return p->pid;
}

// 创建子进程（fork）
int fork_process(void){
  if(!current) return -1;                   // 必须有当前进程才能创建子进程
  struct proc *np = alloc_process();
  if(!np) return -1;
  acquire(&np->lock);
  np->parent = current;
  np->kstack = alloc_page();
  if(!np->kstack)
  { 
    np->state=UNUSED; 
    release(&np->lock); 
    return -1; 
  }
  np->trapframe = (struct trapframe*)alloc_page();
  if(!np->trapframe)
  { 
    free_page(np->kstack); 
    np->kstack=0; 
    np->state=UNUSED; 
    release(&np->lock); 
    return -1; 
  }
  np->pagetable = create_pagetable();
  if(!np->pagetable)
  { 
    free_page(np->trapframe); 
    free_page(np->kstack); 
    np->trapframe=0; 
    np->kstack=0; 
    np->state=UNUSED; 
    release(&np->lock); 
    return -1; 
  }
  // 复制代码页 (assume single page at USER_TEXT_START)
  pte_t *pte = walk_lookup(current->pagetable, USER_TEXT_START);    // 查找代码页的pte
  if(pte && (*pte & PTE_V)){                                        // 如果代码页存在且有效
    void *newpage = alloc_page();
    if(newpage){
      memcpy(newpage, (void*)PTE_PA(*pte), PGSIZE);                 // 复制代码页内容
      map_region(np->pagetable, USER_TEXT_START, (uint64)newpage, PGSIZE, (*pte & (PTE_R|PTE_X)) | PTE_U);  // 映射到子进程页表
    }
  }
  // 复制栈页
  pte = walk_lookup(current->pagetable, USER_STACK_TOP - USER_STACK_SIZE);  // 查找栈页的pte
  if(pte && (*pte & PTE_V)){
    void *newstack = alloc_page();
    if(newstack){
      memcpy(newstack, (void*)PTE_PA(*pte), PGSIZE);
      map_region(np->pagetable, USER_STACK_TOP - USER_STACK_SIZE, (uint64)newstack, PGSIZE, (*pte & (PTE_R|PTE_W)) | PTE_U);
    }
  }
  // 映射 TRAMPOLINE & TRAPFRAME (fork 也需要)
  extern char trampoline[];  // from trampoline.S
  map_region(np->pagetable, TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X | PTE_U);
  map_region(np->pagetable, TRAPFRAME, (uint64)np->trapframe, PGSIZE, PTE_R | PTE_W | PTE_U);
  // trapframe复制
  memcpy(np->trapframe, current->trapframe, sizeof(*np->trapframe));
  np->trapframe->a0 = 0;            // 子进程返回0
  // 补齐必要内核字段
  np->trapframe->kernel_satp = r_satp();
  np->trapframe->kernel_sp = (uint64)np->kstack + PGSIZE;
  np->trapframe->kernel_trap = (uint64)usertrap;
  np->trapframe->kernel_hartid = read_tp();
  np->trapframe->sstatus = r_sstatus() & ~SSTATUS_SPP; // 准备返回用户态
  extern void usertrapret();
  init_context(&np->context, (void*)usertrapret);
  np->context.sp = (uint64)np->kstack + PGSIZE;
  np->is_kernel_thread = 0;
  np->state = RUNNABLE;
  strncpy(np->name, current->name, sizeof(np->name)-1);
  np->name[sizeof(np->name)-1] = '\0';
  release(&np->lock);
  return np->pid;
}

// 杀死指定PID的进程
int kill_process(int pid){
  for(int i=0;i<NPROC;i++){
    acquire(&proc[i].lock);
    if(proc[i].pid == pid && proc[i].state != UNUSED){  // 找到目标进程
      proc[i].killed = 1;
      if(proc[i].state == SLEEPING){  // 如果进程正在睡眠
        proc[i].state = RUNNABLE;    // 唤醒进程
        proc[i].chan = 0;            // 清除睡眠通道避免残留
      }
      release(&proc[i].lock);
      return 0;
    }
    release(&proc[i].lock);
  }
  return -1;
}

// 等待任意子进程退出
int wait_all(void){
  int reaped = 0;                         // 已回收的子进程数量
  int status;                             // 退出状态
  while(1){
    int pid = wait_process(&status);      // 等待子进程退出
    if(pid < 0) break;
    reaped++;                             // 增加已回收计数
  }
  return reaped;
}

// 退出当前进程，释放资源
void exit_process(int status){
  struct proc *p = current;
  if(!p){ dprint("[exit] no current\n"); return; }
  acquire(&p->lock);
  p->xstate = status;
  for(int i=0;i<NPROC;i++){
    if(proc[i].parent == p){ acquire(&proc[i].lock); proc[i].parent = initproc; release(&proc[i].lock); }
  }
  p->state = ZOMBIE;
  dprint("[exit] pid=%d status=%d\n", p->pid, status);
  wakeup(p->parent);
  release(&p->lock);
  yield();
}

// 等待子进程退出
int wait_process(int *status){
  struct proc *cp = current;
  if(!cp){ dprint("[wait] no current\n"); return -1; }
  acquire(&cp->lock);
  for(;;){
    int havekids=0;
    for(int i=0;i<NPROC;i++){
      acquire(&proc[i].lock);
      if(proc[i].parent==cp && proc[i].state!=UNUSED){
        havekids=1;
        if(proc[i].state==ZOMBIE){
          int pid=proc[i].pid;
          if(status) *status = proc[i].xstate;
          if(!proc[i].is_kernel_thread){
            if(proc[i].trapframe){ free_page(proc[i].trapframe); proc[i].trapframe=0; }
            if(proc[i].pagetable){ destroy_pagetable(proc[i].pagetable); proc[i].pagetable=0; }
          }
          if(proc[i].kstack){ free_page(proc[i].kstack); proc[i].kstack=0; }
          proc[i].state=UNUSED; proc[i].parent=0;
          release(&proc[i].lock); release(&cp->lock);
          dprint("[wait] reaped pid=%d status=%d\n", pid, status?*status:0);
          dump_runnable_counts();
          return pid;
        }
      }
      release(&proc[i].lock);
    }
    if(!havekids || cp->killed){ dprint("[wait] done havekids=%d killed=%d\n", havekids, cp->killed); release(&cp->lock); return -1; }
    dprint("[wait] sleep pid=%d\n", cp->pid);
    sleep_chan(cp,&cp->lock);
  }
}

// 设置指定PID进程的优先级
int set_priority(int pid, int prio){  
  if(prio < 1) prio = 1;                  // 最小优先级为1
  if(prio > 100) prio = 100;              // 最大优先级为100
  for(int i=0;i<NPROC;i++){
    acquire(&proc[i].lock);
    if(proc[i].pid == pid && proc[i].state != UNUSED){  // 找到目标进程
      proc[i].priority = prio;                          // 设置当前优先级
      proc[i].base_priority = prio;                     // 设置基准优先级 
      release(&proc[i].lock);
      return 0;
    }
    release(&proc[i].lock);
  }
  return -1;
}

// 让出CPU，进入调度器
void yield(void){
  if(!current){ dprint("[yield] no current\n"); return; }
  dprint("[yield] enter pid=%d sp=%p\n", current->pid, read_sp());
  acquire(&current->lock);
  if(current->state == RUNNING){ current->state = RUNNABLE; dprint("[yield] pid=%d -> RUNNABLE\n", current->pid); }
  release(&current->lock);
  extern void swtch(struct context*, struct context*);
  uint64 t0 = r_time();
  swtch(&current->context, &sched_ctx);
  uint64 t1 = r_time();
  ctx_switch_cycles_sum += (t1 - t0);
  ctx_switch_count++;
  dprint("[yield] return pid_prev=%d cycles=%lu sp=%p\n", current?current->pid:-1, (unsigned long)(t1-t0), read_sp());
}

// 调度器主循环
// 基于优先级的调度算法，基于老化机制
void scheduler(void){
  for(;;){
    intr_on();
    int best_idx=-1; int best_prio=-1;
    for(int i=0;i<NPROC;i++){
      acquire(&proc[i].lock);
      if(proc[i].state==RUNNABLE){
        if(proc[i].priority < proc[i].base_priority + 50) proc[i].priority++;
        if(proc[i].priority > best_prio){ best_prio=proc[i].priority; best_idx=i; }
      }
      release(&proc[i].lock);
    }
    if(best_idx>=0){
      acquire(&proc[best_idx].lock);
      if(proc[best_idx].state==RUNNABLE){
        proc[best_idx].state=RUNNING; current=&proc[best_idx]; proc[best_idx].sched_count++; proc[best_idx].slice_ticks=0;
        dprint("[sched] switch to pid=%d prio=%d sp=%p kstack=%p top=%p\n", proc[best_idx].pid, proc[best_idx].priority, read_sp(), proc[best_idx].kstack, (void*)((uint64)proc[best_idx].kstack+PGSIZE));
        release(&proc[best_idx].lock);
        extern void swtch(struct context*, struct context*);
        swtch(&sched_ctx,&proc[best_idx].context);
        dprint("[sched] back from pid=%d sp=%p current=%d\n", proc[best_idx].pid, read_sp(), current?current->pid:-1);
        acquire(&proc[best_idx].lock);
        if(current && current->state==RUNNING){
          if(current->priority>1) current->priority -= 2; current->state=RUNNABLE; dprint("[sched] post-run demote pid=%d prio=%d sp=%p\n", current->pid, current->priority, read_sp());
        }
        release(&proc[best_idx].lock);
        current=0;
      } else {
        release(&proc[best_idx].lock);
      }
    }
  }
}

// 唤醒在 chan 上睡眠的进程
void wakeup(void *chan){
  for(int i=0;i<NPROC;i++){
    acquire(&proc[i].lock); 
    if(proc[i].state == SLEEPING && proc[i].chan == chan){
      proc[i].state = RUNNABLE;
      proc[i].chan = 0;
    }
    release(&proc[i].lock);
  }
}

// 在 chan 上睡眠，释放锁
void sleep_chan(void *chan, spinlock *lk){
  struct proc *p = current;
  if(!p) return;
  // 如果传入的是当前进程自己的锁，假定已持有，不再重复获取，避免死锁
  if(lk == &p->lock){
    p->chan = chan;
    p->state = SLEEPING;
    release(&p->lock);          // 释放锁，允许其他进程修改/唤醒
    yield();                    // 调度
    acquire(&p->lock);          // 醒来后重新获取
    return;
  }
  // 正常路径: lk 是外部锁
  acquire(&p->lock);
  release(lk);
  p->chan = chan;
  p->state = SLEEPING;
  release(&p->lock);
  yield();
  acquire(lk);
}

// 调试：打印进程表信息
void debug_proc_table(void){
  printf("=== Process Table ===\n");
  for(int i=0;i<NPROC;i++){
    struct proc *p = &proc[i];
    if(p == current) { 
        if(p->state != UNUSED){
          printf("PID:%d State:%s Name:%s Parent:%d run=%lu sched=%lu slice=%u (current)\n", p->pid, proc_state_name(p->state), p->name, p->parent?p->parent->pid:-1, p->run_ticks, p->sched_count, p->slice_ticks);
        }
        continue;
    }
    acquire(&p->lock);
    if(p->state != UNUSED){
      printf("PID:%d State:%s Name:%s Parent:%d run=%lu sched=%lu slice=%u\n", p->pid, proc_state_name(p->state), p->name, p->parent?p->parent->pid:-1, p->run_ticks, p->sched_count, p->slice_ticks);
    }
    release(&p->lock);
  }
}

// 进程状态统计与调试输出
void dump_runnable_counts(void){
  int unused=0, used=0, runnable=0, running=0, sleeping=0, zombie=0;
  for(int i=0;i<NPROC;i++){
    struct proc *p = &proc[i];
    if(p == current) {
        switch(p->state){
          case UNUSED: unused++; break;
          case USED: used++; break;
          case RUNNABLE: runnable++; break;
          case RUNNING: running++; break;
          case SLEEPING: sleeping++; break;
          case ZOMBIE: zombie++; break;
        }
        continue;
    }
    acquire(&p->lock);
    switch(p->state){
      case UNUSED: unused++; break;
      case USED: used++; break;
      case RUNNABLE: runnable++; break;
      case RUNNING: running++; break;
      case SLEEPING: sleeping++; break;
      case ZOMBIE: zombie++; break;
    }
    release(&p->lock);
  }
  dprint("[dump] unused=%d used=%d runnable=%d running=%d sleeping=%d zombie=%d nextpid=%d current=%d\n", unused, used, runnable, running, sleeping, zombie, nextpid, current?current->pid:-1);
  if(used || runnable || running || sleeping || zombie){
    for(int i=0;i<NPROC;i++){
      struct proc *p = &proc[i];
      if(p == current) {
        if(p->state!=UNUSED){
            dprint("[dump] pid=%d state=%s parent=%d kstack=%p trapframe=%p pagetable=%p name=%s (current)\n",
              p->pid, proc_state_name(p->state), p->parent?p->parent->pid:-1,
              p->kstack, p->trapframe, p->pagetable, p->name);
        }
        continue;
      }
      acquire(&p->lock);
      if(p->state!=UNUSED){
        dprint("[dump] pid=%d state=%s parent=%d kstack=%p trapframe=%p pagetable=%p name=%s\n",
          p->pid, proc_state_name(p->state), p->parent?p->parent->pid:-1,
          p->kstack, p->trapframe, p->pagetable, p->name);
      }
      release(&p->lock);
    }
  }
}
