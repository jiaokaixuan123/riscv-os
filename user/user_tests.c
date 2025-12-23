#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

// 简单的断言宏
#define FS_ASSERT(expr, msg) do { if(!(expr)) { printf("[fs-tests] ASSERT FAIL 断言失败: %s (%s)\n", #expr, msg); exit(1); } } while(0)

// 创建、写入、读取并验证一个文件，然后删除它
static void test_filesystem_integrity(void) {
  printf("[fs-tests] Integrity 文件完整性测试: start 开始\n");
  int fd = open("testfile", O_CREATE | O_RDWR);
  FS_ASSERT(fd >= 0, "open create testfile");
  char buffer[] = "Hello, filesystem!";
  int bytes = write(fd, buffer, strlen(buffer));
  FS_ASSERT(bytes == (int)strlen(buffer), "write testfile");
  close(fd);

  fd = open("testfile", O_RDONLY);
  FS_ASSERT(fd >= 0, "reopen testfile");
  char readbuf[64];
  bytes = read(fd, readbuf, sizeof(readbuf)-1);
  FS_ASSERT(bytes > 0, "read testfile");
  readbuf[bytes] = '\0';
  FS_ASSERT(strcmp(buffer, readbuf) == 0, "content mismatch");
  close(fd);
  FS_ASSERT(unlink("testfile") == 0, "unlink testfile");
  printf("[fs-tests] Integrity: created/wrote/verified/unlinked 完成\n");
}

// 并发访问测试
static void test_concurrent_access(void) {
  printf("[fs-tests] Concurrency 并发测试: start 开始\n");
  int nchild = 4;
  printf("[fs-tests] Concurrency: forking=%d 开始创建子进程\n", nchild);
  for(int i=0;i<nchild;i++) {
    int pid = fork();
    if(pid == 0) {
      char fname[32];
      // 每个子进程创建100个文件
      for(int j=0;j<100;j++) {
        int len = 0;
        // 格式: test_0_00, test_0_01, ..., test_3_99
        fname[len++]='t'; fname[len++]='e'; fname[len++]='s'; fname[len++]='t'; fname[len++]='_';
        fname[len++]='0'+(i%10); fname[len++]='_';
        fname[len++]='0'+((j/10)%10); fname[len++]='0'+(j%10); fname[len]='\0';
        int fd = open(fname, O_CREATE | O_RDWR);  // 创建并打开文件
        if(fd >= 0) { 
          write(fd, (char*)&j, sizeof(j));        // 写入数据
          close(fd); 
          unlink(fname);                          
        }
      }
      // 子进程只打印一条短行，避免交叉
      printf("[fs-tests] Concurrency: child 完成 pid=%d done\n", getpid());
      exit(0);
    }
  }
  for(int i=0;i<nchild;i++) wait(0);
  printf("[fs-tests] Concurrency: all workers finished 全部完成\n");
}

// 文件系统性能测试
static void test_filesystem_performance(void) {
  printf("[fs-tests] Performance 性能测试: start 开始\n");
  int small_count = 100;
  printf("[fs-tests] Performance: small-files 小文件=%d\n", small_count);
  int t0 = uptime();
  for(int i=0;i<small_count;i++) {
    char fname[24];                   // 生成文件名
    int len=0; 
    // 格式: small_000, small_001, ...
    fname[len++]='s';      
    fname[len++]='m';                 
    fname[len++]='a'; 
    fname[len++]='l'; 
    fname[len++]='l'; 
    fname[len++]='_';
    fname[len++]='0'+((i/100)%10); 
    fname[len++]='0'+((i/10)%10); 
    fname[len++]='0'+(i%10); 
    fname[len]='\0';
    int fd = open(fname, O_CREATE | O_RDWR);
    if(fd >= 0){ 
      write(fd, "test", 4);   // 写入数据test
      close(fd); 
      unlink(fname); 
    } else { 
      printf("[fs-tests] Performance: create fail 失败 %s\n", fname); 
    }
  }
  int t1 = uptime();

  // 大文件测试
  int fd = open("bigfile", O_CREATE | O_RDWR);
  if(fd < 0) { 
    printf("[fs-tests] Performance: open bigfile fail 打开失败\n"); 
    printf("[fs-tests] Performance: early stop 提前结束\n"); 
    return; 
  }
  char blk[512];
  for(int i=0;i<(64*1024)/sizeof(blk); i++) {     // 写入 64KB 数据
    write(fd, blk, sizeof(blk)); 
  }
  close(fd);
  int t2 = uptime();  
  printf("[fs-tests] Performance: small=%d, ticks 小文件=%d, large 大文件=%d\n", small_count, t1 - t0, t2 - t1);
}

// Timer 时钟测试
static void test_timer_interrupt(void) {
  printf("[Timer-tests] Timer 时钟测试: start 开始\n");
  uint64 start = uptime();
  int ticks = 0;
  uint64 last = start;
  while (ticks < 5) {
    while (1) {
      uint64 now = uptime();
      if (now > last) { ticks++; printf("[Timer-tests] Timer: tick=%d uptime=%lu\n", ticks, now); last = now; break; }
      for (volatile int k = 0; k < 20000; k++) {}
    }
  }
  uint64 end = uptime();
  printf("[Timer-tests] Timer: ticks=%d elapsed 用时=%lu\n", ticks, end - start);
}

// 异常处理测试
static void test_exception_handling(void) {
  printf("[exception-tests] Exception 异常测试: start 开始\n");
  printf("[exception-tests] Exception: note 注意 - user exceptions 进程会退出\n");
  printf("[exception-tests] Exception: placeholder 仅占位\n");
}

// 进程创建测试
static void test_process_creation(void) {
  printf("[Proc-tests] Process 进程创建(并行): start 开始\n");
  int pids[128];
  int created = 0;
  int target = 64 + 5;
  for (int i = 0; i < target; i++) {
    int pid = fork();
    if (pid < 0) { printf("[Proc-tests] Process: fork fail 在第%d次失败\n", i); break; }
    else if (pid == 0) {
      // 子进程不打印，避免与其他输出交叉
      for (volatile int k = 0; k < 200000; k++) {}
      exit(0);
    } else {
      if (created < (int)(sizeof(pids)/sizeof(pids[0]))) { pids[created++] = pid; }
    }
  }
  printf("[Proc-tests] Process: spawned 创建=%d, waiting 等待...\n", created);
  for (int i = 0; i < created; i++) {
    wait(0);
    if ((i+1) % 8 == 0 || i+1 == created) printf("[Proc-tests] Process: reaped 已回收=%d/%d\n", i+1, created);
  }
  printf("[Proc-tests] Process: done 完成 spawned=%d reaped=%d\n", created, created);
}

static void cpu_intensive_task(void) { // 简单的计算密集型工作
  uint64 start = uptime();
  while (uptime() - start < 50) {                 // 运行50个滴答
    for (volatile int k = 0; k < 100000; k++) {}  // 空循环，模拟工作
  }
}

// 本地基于 uptime 的简单睡眠，避免对未声明的 sleep 依赖
static void msleep(uint64 ticks) {
  uint64 start = uptime();
  while (uptime() - start < ticks) {
    for (volatile int k = 0; k < 20000; k++) {}
  }
}

// 调度器测试
static void test_scheduler(void) { 
  printf("[Sche-tests] Testing scheduler...\n"); 
  int procs = 3; 
  for (int i = 0; i < procs; i++) { 
    int pid = fork(); 
    if (pid == 0) { cpu_intensive_task(); exit(0); } 
  }
  uint64 start_time = uptime(); 
  // 用 msleep 代替 sleep，按滴答数等待
  msleep(100); 
  uint64 end_time = uptime(); 
  for (int i = 0; i < procs; i++) wait(0);
  printf("[Sche-tests] Scheduler test completed in %lu ticks\n", end_time - start_time);
}

// 同步测试:生产者-消费者场景
static void test_synchronization(void) {
  printf("[Sync-tests] Sync 同步(管道): start 开始\n");
  int fds[2];                                   
  FS_ASSERT(pipe(fds) == 0, "pipe");            // 创建管道
  int pid = fork();
  if (pid == 0) {                               // 子进程:消费者
    close(fds[1]);                              // 关闭写端
    char c; int got = 0;
    while (read(fds[0], &c, 1) == 1) { got++; } // 读取数据
    close(fds[0]);                              // 关闭读端
    printf("[Sync-tests] Sync: consumer 总计 got=%d\n", got);
    exit(0);
  } else if (pid > 0) {                         // 父进程:生产者
    close(fds[0]);                              // 关闭读端
    for (int i = 0; i < 100; i++) { if (write(fds[1], "X", 1) != 1) break; }  // 写入数据
    close(fds[1]);                              // 关闭写端
    wait(0);                                    // 等待子进程
    printf("[Sync-tests] Sync: done 完成\n");
  } else {
    printf("[Sync-tests] Sync: fork 失败 fail\n");
  }
}

// 中断频率对用户态性能的影响（tick级粗测）
static void test_interrupt_freq_effect_user(void) {
  printf("[Timer-tests] 用户态中断频率影响\n");
  // 记录多轮 tick 间隔
  int rounds = 5; uint64 last = uptime(); uint64 total = 0;
  for(int i=0;i<rounds;i++){
    while(1){ uint64 now = uptime(); if(now>last){ total += (now-last); last = now; break; }
      for(volatile int k=0;k<20000;k++){}
    }
  }
  printf("用户态: 连续 %d 次 tick 平均间隔=%lu ticks\n", rounds, total/rounds);
}

// 上下文切换成本（用户态粗测：多子进程+等待）
static void test_ctxswitch_cost_user(void) {
  printf("[Sche-tests] 用户态上下文切换成本\n");
  int procs = 4; 
  uint64 t0 = uptime();
  for(int i=0;i<procs;i++){
    int pid=fork();
    if(pid==0){ // 子进程做短工作
      for(volatile int k=0;k<300000;k++){} 
      exit(0);
    }
  }
  for(int i=0;i<procs;i++) wait(0);
  uint64 t1 = uptime();
  printf("用户态: 创建+等待 %d 进程 用时=%lu ticks\n", procs, t1-t0);
}

// 崩溃恢复测试：分两阶段运行
// phase1：大量创建/写入文件后直接退出，模拟崩溃
// phase2：重启后运行，检查文件系统是否一致（文件/目录可访问，无半写入的元数据）
static void test_crash_recovery(int phase) {
  if (phase == 1) {
    printf("[Crash-tests] Phase1(写入期间崩溃) 创建文件: start 开始\n");
    mkdir("crashdir");
    if (chdir("crashdir") < 0) { printf("[Crash-tests] Phase1: chdir fail\n"); return; }

    int files = 64;   // 适中数量
    for (int i = 0; i < files; i++) {
      char name[32]; int len = 0;
      name[len++]='f'; name[len++]='i'; name[len++]='l'; name[len++]='e'; name[len++]='_';
      name[len++]='0'+((i/10)%10); name[len++]='0'+(i%10); name[len]='\0';
      int fd = open(name, O_CREATE | O_RDWR);
      if (fd < 0) { printf("[Crash-tests] Phase1: open fail %s\n", name); continue; }

      // 分块写入同一个文件，在写入过程中插入短暂忙等待，便于你在"正在写入"时重启
      char blk[512]; for (int w = 0; w < (int)sizeof(blk); w++) blk[w] = 'A' + (i % 26);
      int chunks = 64; // 每文件写 64 个块，共 32KB
      for (int c = 0; c < chunks; c++) {
        int wr = write(fd, blk, sizeof(blk));
        if (wr != (int)sizeof(blk)) { printf("[Crash-tests] Phase1: write short %s c=%d wr=%d\n", name, c, wr); break; }
        // 在每几个块后停顿一小段时间，制造可观察窗口（此时仍在写入该文件）
        if (c % 32 == 0) {
          // 打点提示：当前正在写入哪个文件与块序号
          printf("[Crash-tests] Phase1: 写入进行中 file=%s chunk=%d/%d\n", name, c, chunks);
          // 短暂忙等待（保持进程活跃，仍在写入循环中）
          uint64 start = uptime();
          while (uptime() - start < 5) { for (volatile int k = 0; k < 1000; k++) {} }
        }
      }
      close(fd);
    }

    // 保活不退出，继续进行小写入循环，确保任意时刻都可能发生在写入期间
    printf("[Crash-tests] Phase1: 主循环结束，进入持续写入保活（随时可 reboot）。\n");
    int idx = 0; char keep[512]; for (int w = 0; w < (int)sizeof(keep); w++) keep[w] = 'K';
    while (1) {
      // 循环向同一个文件追加写入，制造持续的写入行为
      int fd = open("file_keep", O_CREATE | O_RDWR);
      if (fd >= 0) {
        // 追加到末尾
        int chunks2 = 16;
        for (int c = 0; c < chunks2; c++) {
          int wr = write(fd, keep, sizeof(keep));
          if (wr != (int)sizeof(keep)) break;
          if (c % 4 == 0) {
            printf("[Crash-tests] Phase1: 保活写入中 idx=%d chunk=%d/%d (可 reboot)\n", idx, c, chunks2);
            uint64 start = uptime();
            while (uptime() - start < 20) { for (volatile int k = 0; k < 50000; k++) {} }
          }
        }
        close(fd);
      }
      idx++;
    }
  } else if (phase == 2) {
    printf("[Crash-tests] Phase2(重启后校验): start 开始\n");
    if (chdir("crashdir") < 0) { printf("[Crash-tests] Phase2: chdir crashdir 失败\n"); return; }

    // 检查固定集合文件完整性与大小
    int files = 64; int present = 0, zero = 0, full = 0, partial = 0; // partial: 非0且非整块倍数
    for (int i = 0; i < files; i++) {
      char name[32]; int len = 0;
      name[len++]='f'; name[len++]='i'; name[len++]='l'; name[len++]='e'; name[len++]='_';
      name[len++]='0'+((i/10)%10); name[len++]='0'+(i%10); name[len]='\0';
      int fd = open(name, O_RDONLY);
      if (fd < 0) continue; // 可能被回滚
      present++;
      char buf[512]; int r = read(fd, buf, sizeof(buf));
      // 只读第一块大小作为示意
      if (r == 0) zero++;
      else if (r == (int)sizeof(buf)) full++; // 如果读到整块大小，视为完整
      else partial++;   // 否则视为部分写入
      close(fd);
    }

    // 对保活写入文件进行大小检查（读少量以示例）；允许存在多种结果
    int fd = open("file_keep", O_RDONLY);
    if (fd >= 0) {
      char b[512]; int r = read(fd, b, sizeof(b)); close(fd);
      printf("[Crash-tests] Phase2: file_keep 首块读取=%d\n", r);
    }

    printf("[Crash-tests] Phase2: present=%d full=%d zero=%d partial=%d\n", present, full, zero, partial);
  }
}

int main(int argc, char *argv[]) {
  printf("[OS-tests] BEGIN 开始\n");
  if (argc >= 2) {
    if (strcmp(argv[1], "crash1") == 0) { test_crash_recovery(1); for(;;){} }
    if (strcmp(argv[1], "crash2") == 0) { test_crash_recovery(2); printf("[OS-tests] END 结束\n"); exit(0); }
  }
  test_filesystem_integrity();
  test_concurrent_access();
  test_filesystem_performance();
  test_timer_interrupt();
  test_exception_handling();
  test_process_creation();
  test_scheduler();
  test_synchronization();
  test_interrupt_freq_effect_user();
  test_ctxswitch_cost_user();
  printf("[OS-tests] END 结束\n");
  exit(0);
}
