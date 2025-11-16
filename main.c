#include "types.h"
#include "printf.h"
#include "uart.h"
#include "console.h"
#include "kalloc.h"
#include "vm.h"
#include "riscv.h"
#include "defs.h" // 添加：提供trapinit/get_ticks等声明
#include "interrupt.h"
#include "proc.h"
#include "memlayout.h" // for USER_MAX_VA
extern void manager_task();
extern volatile uint64 timer_interval; // 来自 trap.c
extern volatile uint64 isr_cycles_sum;
extern volatile uint64 isr_count;

void test_printf_basic() {
  printf("Testing integer: %d\n", 42);
  printf("Testing negative: %d\n", -123);
  printf("Testing zero: %d\n", 0);
  printf("Testing hex: 0x%x\n", 0xABC);
  printf("Testing string: %s\n", "Hello");
  printf("Testing char: %c\n", 'X');
  printf("Testing percent: %%\n");
}
void test_printf_edge_cases() {
  printf("INT_MAX: %d\n", 2147483647);
  printf("INT_MIN: %d\n", -2147483648);
  printf("NULL string: %s\n", (char*)0);
  printf("Empty string: %s\n", "");
}

static void test_console_clear_and_cursor(void) {
  // 1) 清屏
  clear_screen();
  printf("== Screen cleared by ANSI ESC ==\n");

  // 2) 光标定位（把“Title”放到第 3 行第 10 列）
  goto_xy(10, 2);  // x,y 都是从 0 开始计；内部已转换为 1-based
  printf("Title @ (10,2)\n");

  // 3) 颜色输出示例（绿、黄、红）
  printf_color(COLOR_GREEN, "Green OK\n");
  printf_color(COLOR_YELLOW, "Yellow Note\n");
  printf_color(COLOR_RED, "Red Warn\n");

  // 4) 清当前行
  printf("This line will be cleared...");
  clear_line();
  printf("\n[done] clear_line()\n");
}

void test_physical_memory(void) {
  void *page1 = alloc_page();
  void *page2 = alloc_page();
  assert(page1 != page2);
  assert(((uint64)page1 & 0xFFF) == 0);  // 页对齐检查
  // 测试数据写入
  *(int*)page1 = 0x12345678; 
  assert(*(int*)page1 == 0x12345678);

  free_page(page1);
  void *page3 = alloc_page();

  free_page(page2);
  free_page(page3);

}

void test_pagetable(void) {
  pagetable_t pt = create_pagetable();

  // 测试基本映射4
  uint64 va = 0x1000000; 
  uint64 pa = (uint64)alloc_page(); 
  assert(map_page(pt, va, pa, PTE_R | PTE_W) == 0);

  // 测试地址转换
  pte_t *pte = walk_lookup(pt, va); 
  assert(pte != 0 && (*pte & PTE_V)); 
  assert(PTE_PA(*pte) == pa);

  // 测试权限位14
  assert(*pte & PTE_R); 
  assert(*pte & PTE_W); 
  assert(!(*pte & PTE_X));
}

// 测试启用分页后的内存访问
void test_virtual_memory(void) {
  printf("Before enabling paging...\n");
  // 启用分页
  kvminit();
  kvminithart();
  printf("After enabling paging...\n");
    // 测试内核代码仍然可执行
    // 测试内核数据仍然可访问
    // 测试设备访问仍然正常
  printf("paging enabled. satp=%p\n", r_satp());

  //测试console
  printf("still printing after paging, good!\n");
}

// 测试定时器中断
static void test_timer_interrupt(void){
  printf("Testing timer interrupt...\n");

  // 记录中断前时间和起始中断次数
  uint64 start_time = get_time();
  int start_interrupts = timer_interrupts;  // 记录起始中断次数

  while ((timer_interrupts - start_interrupts) < 5) { 
    // 执行其他任务：计算阶乘（模拟计算密集型任务）
    static int n = 1;
    int fact = 1;
    for (int i = 1; i <= n; i++) {
      fact *= i;
    }
    printf("Factorial of %d is %d (interrupts so far: %d)\n", 
           n, fact, timer_interrupts - start_interrupts);
    n++;
    
    // 简单延时（模拟 I/O 或其他等待）
    for (volatile int i = 0; i < 10000000; i++); 
  }

  uint64 end_time = get_time();

  printf("Timer test completed: %d interrupts in %lu cycles\n",
          timer_interrupts - start_interrupts, end_time - start_time);
}

// 测试异常处理
static void test_exception_handling(void){
  printf("Testing exception handling...\n");
  // 注意: RISC-V 算术除零不会产生异常, 结果是实现定义或按规范返回全1。
  volatile int zero = 0;
  volatile int divres = 123 / zero; // 不会触发 trap
  printf("Divide by zero (no trap per RISC-V spec): %d\n", divres);

  // 1) 非法指令 (illegal instruction)
  // printf("Triggering illegal instruction...\n");
  // asm volatile(".word 0xffffffff" ::: "memory");
  // printf("Returned from illegal instruction trap (sepc advanced)\n");

  // 2) 断点异常 (breakpoint ebreak)
  printf("Triggering breakpoint (ebreak)...\n");
  asm volatile("ebreak" ::: "memory");
  printf("Returned from breakpoint trap\n");

  // 3) S 模式 ecall (environment call from S-mode cause=9)
  printf("Triggering S-mode ecall...\n");
  asm volatile("ecall" ::: "memory");
  printf("Returned from S-mode ecall trap\n");

  // 4) 加载地址未对齐 (load addr misaligned)
  void *pg = alloc_page();
  volatile unsigned int *mis_l = (unsigned int*)((char*)pg + 2); // 故意制造未对齐 (4字节访问地址+2)
  unsigned int lv = *mis_l; (void)lv;
  printf("After misaligned load\n");

  // 5) 存储地址未对齐 (store addr misaligned)
  volatile unsigned int *mis_s = (unsigned int*)((char*)pg + 2);
  *mis_s = 0xdeadbeef;
  printf("After misaligned store\n");
  free_page(pg);

  // 可选: 页错误测试 (可能导致 panic, 放最后)
  #define TEST_PAGE_FAULTS 0
  if(TEST_PAGE_FAULTS){
    // 6) 加载页错误 (load page fault)
    printf("Triggering load page fault...\n");
    volatile int *bad_load = (int*)0xFFFFFFFFFFFFULL; // 非规范/未映射地址
    int v = *bad_load; (void)v;

    // 7) 存储页错误 (store page fault)
    printf("Triggering store page fault...\n");
    volatile int *bad_store = (int*)0xFFFFFFFFFFFEULL;
    *bad_store = 1;
  }

  printf("Exception tests completed (page fault tests may be disabled)\n");
}

// 测量中断处理的时间开销
static void test_timer_interrupt_cost(void){
  printf("Testing timer interrupt's costs...\n");
  // 采用较多次定时器中断测平均: 使用当前周期与计数
  uint64 start_cycles = get_time();
  uint64 start_ticks = get_ticks();
  uint64 start_int = timer_interrupts;
  int target = 10;                // 目标中断次数
  while((int)(timer_interrupts - start_int) < target){
    asm volatile("wfi");          // 等待中断
    if((timer_interrupts - start_int) % 1 == 0) 
      printf("progress: %d\n", (int)(timer_interrupts - start_int));
  }
  uint64 end_cycles = get_time();
  uint64 end_ticks = get_ticks();
  int got = (int)(timer_interrupts - start_int); 
  uint64 cyc = end_cycles - start_cycles;   

  printf("Overhead: "); 
  printf("%d ", got);                                                     // 中断次数
  printf("interrupts, cycles=");  
  printf("%lu ", (unsigned long)cyc);                                     // 总周期数
  printf("avg=%lu ", (unsigned long)(cyc/got));                           // 平均每次中断周期数
  printf("ticks_delta=%lu\n", (unsigned long)(end_ticks - start_ticks));  // 总ticks增量
}

// 测量单次上下文切换的时间成本
static struct context origin_ctx; // 静态，避免局部栈失效
static struct context A_ctx;      // 静态
static void ctx_entry(){          // A 的入口：切回 origin
  extern void swtch(struct context*, struct context*);
  swtch(&A_ctx, &origin_ctx);     // A -> origin
}
static __attribute__((noinline)) uint64 measure_one_swtch(void){
  extern void swtch(struct context*, struct context*);
  void *sA = alloc_page(); if(!sA) return 0;              // 分配 A 的栈
  // 重置上下文结构
  memset(&origin_ctx, 0, sizeof(origin_ctx));             // origin_ctx 清零
  memset(&A_ctx, 0, sizeof(A_ctx));                       // A_ctx 清零
  A_ctx.sp = (uint64)sA + PGSIZE;                         // A 的栈顶
  A_ctx.ra = (uint64)ctx_entry;                           // 进入 A 后执行 ctx_entry，再切回
  asm volatile("mv %0, sp" : "=r"(origin_ctx.sp));        // origin_ctx.sp = 当前 sp

  uint64 old_sie = r_sie(); w_sie(old_sie & ~SIE_STIE);   // 关定时器中断避免抖动
  asm volatile("fence iorw, iorw" ::: "memory");          // 防止指令重排
  uint64 t0 = get_time();                                 // 记录开始时间 
  swtch(&origin_ctx, &A_ctx);                             // origin -> A (A 立即再切回)
  uint64 t1 = get_time();                                 // 回来后继续
  asm volatile("fence iorw, iorw" ::: "memory");          // 防止指令重排
  w_sie(old_sie);                                         // 恢复中断
  free_page(sA);                                          // 释放栈
  return t1 - t0;                                         // 两次上下文切换总周期
}

// 测试上下文切换成本
static void test_context_switch_cost(int loops){
  printf("Measuring context switch cost loops=%d...\n", loops);
  int warmup = loops/10; if(warmup < 5) warmup = 5; if(warmup > 50) warmup = 50;    // 热身次数
  printf("1 warming up...\n");
  for(int i=0;i<warmup;i++) { 
    (void)measure_one_swtch();                            // 热身，避免测量偏差
  }
  printf("2 measuring...\n");
  uint64 sum = 0;
  for(int i=0;i<loops;i++){ 
    sum += measure_one_swtch();                           // 累计总周期
    if((i+1) % (loops < 10 ? 1 : loops/10)==0) {
      printf(" progress %d/%d\n", i+1, loops);            // 进度输出
    }
  }
  printf("3 done.\n");
  uint64 avg_two = sum/loops; // 两次切换平均
  uint64 avg_one = avg_two/2; // 单次切换估算
  printf("4");
  printf("ctx_switch: total_cycles=%lu avg_two=%lu avg_one=%lu loops=%d warmup=%d\n",
         (unsigned long)sum, (unsigned long)avg_two, (unsigned long)avg_one, loops, warmup);
}

// 测试中断频率对系统性能的影响
static void test_interrupt_frequency(void){
  printf("Testing interrupt frequency impact...\n");
  reset_isr_stats();                                      // 重置统计数据
  uint64 base_interval = timer_interval;                  // 保存基准间隔
  for(int scale=1; scale<=5; scale++){                    // 多档频率
    set_timer_interval(base_interval/scale);              // 设置更高频率
    reset_isr_stats();                                    // 重置统计数据
    uint64 start = get_time();
    uint64 start_int = timer_interrupts;
    while(timer_interrupts - start_int < 50){ asm volatile("wfi"); }      // 等待50次中断
    uint64 end = get_time();
    uint64 elapsed = end - start;
    uint64 count = isr_count; uint64 isr_cyc = isr_cycles_sum;
    printf("freq_scale=%d interval=%lu cycles: total=%lu ints=%lu avg_gap=%lu isr_avg=%lu overhead_pct=%lu%%\n",
      scale, (unsigned long)(timer_interval), (unsigned long)elapsed, (unsigned long)count,
      (unsigned long)(elapsed/count), (unsigned long)(isr_cyc/count), (unsigned long)(isr_cyc*100/elapsed));
  }
  set_timer_interval(base_interval); // 恢复
}

// 性能测试
static void test_interrupt_overhead(void){
  test_timer_interrupt_cost();                 // 测试定时器中断
  test_context_switch_cost(10);           // 测试上下文切换成本
  test_interrupt_frequency();             // 测试中断频率影响
}

void demo_task(){
  for(int i=0;i<5;i++){
    printf("[demo_task] iteration %d pid? (no user pid retrieval yet)\n", i);
    for(volatile int k=0;k<500000;k++); // busy
    yield();
  }
  exit_process(0);
}

static void user_exception_demo(){
  // 用户态触发异常：非法指令 + 访问未映射内存 + 页懒分配测试
  // 非法指令
  asm volatile(".word 0xffffffff" ::: "memory");
  // 未映射读触发懒分配 (地址在用户空间低端新页)
  volatile int *p = (int*)0x20000; // 未映射页（假设）
  int v = *p; (void)v;
  // 存储触发懒分配
  volatile int *q = (int*)0x21000;
  *q = 123;
  // 故意访问超出用户最大地址杀进程
  volatile int *bad = (int*)(USER_MAX_VA + 0x1000);
  v = *bad; (void)v;
  // 永不达此
  for(;;){}
}

// 测试任务所有的函数定义
#define TEST_BUF_SIZE 8
static int tbuf[TEST_BUF_SIZE];
static int thead=0, ttail=0, tcount=0; 
static int tTARGET_ITEMS = 10; 
static spinlock tbuf_lock; 
static int chan_full_flag; 
static int chan_empty_flag;
void shared_buffer_init(){ initlock(&tbuf_lock); thead=ttail=tcount=0; chan_full_flag=0; chan_empty_flag=1; }
static void tbuffer_put(int x){ acquire(&tbuf_lock); while(tcount==TEST_BUF_SIZE){ sleep_chan(&chan_full_flag, &tbuf_lock); } tbuf[ttail]=x; ttail=(ttail+1)%TEST_BUF_SIZE; tcount++; wakeup(&chan_empty_flag); release(&tbuf_lock); }
static int tbuffer_get(){ acquire(&tbuf_lock); while(tcount==0){ sleep_chan(&chan_empty_flag, &tbuf_lock); } int v=tbuf[thead]; thead=(thead+1)%TEST_BUF_SIZE; tcount--; wakeup(&chan_full_flag); release(&tbuf_lock); return v; }
void producer_task(){ for(int i=0;i<tTARGET_ITEMS;i++){ tbuffer_put(i); printf("[producer] put %d (count=%d)\n", i, tcount); } exit_process(0); }
void consumer_task(){ for(int i=0;i<tTARGET_ITEMS;i++){ int v=tbuffer_get(); printf("[consumer] got %d (count=%d)\n", v, tcount); } exit_process(0); }
void simple_task(){ printf("[simple_task] pid=%d run once and exit\n", current?current->pid:-1); exit_process(0); }
void cpu_intensive_task(){ for(int i=0;i<200000;i++){ if((i%40000)==0){ printf("[cpu_task pid=%d] progress %d\n", current?current->pid:-1, i); yield(); } } exit_process(0); }

// 进程创建测试
void test_process_creation(void) {
  printf("Testing process creation...\n");
  int pid = create_process(simple_task, "simple");              // 测试创建单个进程
  assert(pid > 0);
  int pids[NPROC];
  int count = 0;  
  for (int i = 0; i < 5; i++) {                         // 测试创建超过最大进程数
    int pid2 = create_process(simple_task, "simple");
    if (pid2 > 0) { 
      pids[count++] = pid2; 
    } else { break; }
  }
  printf("Created %d processes, control will be returned to run_all_tests to wait for them.\n", count);
  // Let run_all_tests wait for all children together
  // for (int i = 0; i < count; i++) { wait_process(NULL); }       // 等待所有子进程结束
 }
// 调度器测试
void test_scheduler(void) {
  printf("Testing scheduler...\n");
  for (int i = 0; i < 3; i++) {
      create_process(cpu_intensive_task, "cpu");                // CPU 密集型任务
  }
  uint64 start_time = get_time();
  sleep(1000);
  uint64 end_time = get_time();
  printf("Scheduler test completed in %lu cycles\n", end_time - start_time);
}
//同步机制测试
void test_synchronization(void) {
  shared_buffer_init();                                         // 初始化共享缓冲区
  create_process(producer_task, "prod");
  create_process(consumer_task, "cons");
  // Let run_all_tests wait for all children together
  // wait_process(NULL);
  // wait_process(NULL);
  printf("Synchronization test completed\n");
}

// Stub to avoid undefined reference if manager_task was removed
void manager_task(){ printf("[manager_task stub]\n"); exit_process(0); }
// Kernel thread to run all tests (Approach A)
static void run_all_tests(void){
  printf("[tests] start]\n");
  test_process_creation();
  test_scheduler();
  test_synchronization();
  int status; int reaped=0; int pid;
  while((pid = wait_process(&status)) > 0){
    printf("[tests] reaped pid=%d status=%d\n", pid, status);
    reaped++;
  }
  printf("[tests] all children reaped=%d\n", reaped);
  debug_proc_table();
  exit_process(0);
}

 
int main(void) {
  // volatile char *uart = (char*)0x10000000;
  // *uart = 'S';
  // printf("BOOT STAGE\n");
  // uart_init();
  // clear_screen();
  // printf("Hello from kernel main()\n");
  

  // // ==== 实验2：printf/清屏测试 ====
  // printf("[exp2] basic printf tests begin\n");
  // test_printf_basic();
  // test_printf_edge_cases();
  // printf("[exp2] clear/cursor/color demo begins in 1s...\n");
  // // 简易延时（busy-wait），避免输出过快
  // for (volatile int i = 0; i < 2000000; ++i) {}
  // test_console_clear_and_cursor();
  // printf("[exp2] done\n\n");

  //PMM
  pmm_init();
  test_physical_memory();

  //VM
  test_pagetable();

  test_virtual_memory();

  // ==== 实验4：中断与时钟管理初始化 ====
  trap_init();
  w_sie(r_sie() | SIE_STIE);              // 允许 S 模式的定时器中断
  test_timer_interrupt();                 // 测试定时器中断
  test_exception_handling();              // 测试异常处理
  test_interrupt_overhead();              // 测试中断开销


  // ==== 实验5：进程管理与调度器测试  ====
  proc_init();                            // 初始化进程表 
  create_process(run_all_tests, "tests"); // 创建测试管理线程
  scheduler();                            // 进入调度器 (不会返回)
  while(1);
  return 0;
}
