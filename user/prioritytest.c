#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// 测试优先级调度和老化机制

void
worker(const char *name, int priority, int duration)
{
  int pid = getpid();
  
  setpriority(priority);
  
  printf("Process %s (pid=%d) started with priority %d\n", name, pid, priority);
  
  // 运行指定时间
  int start = uptime();
  int count = 0;
  while(uptime() - start < duration) {
    count++;
    // 模拟一些工作负载
    for(int i = 0; i < 10000; i++) {
      asm volatile("nop");
    }
  }
  
  int current_priority = getpriority();
  printf("Process %s (pid=%d) finished. Count=%d, Final priority=%d\n", 
         name, pid, count, current_priority);
}

int
main(int argc, char *argv[])
{
  printf("Priority Scheduling Test with Aging\n");
  printf("====================================\n");
  printf("Priority range: 0 (highest) to 20 (lowest)\n");
  printf("Aging: waiting processes get higher priority over time\n\n");
  
  int duration = 50; // 每个进程运行的时间（tick 数）
  
  // 创建三个不同优先级的进程
  int pid1 = fork();
  if(pid1 == 0) {
    worker("HIGH", 0, duration);  // Highest priority
    exit(0);
  }
  
  int pid2 = fork();
  if(pid2 == 0) {
    worker("MED", 10, duration);  // Medium priority
    exit(0);
  }
  
  int pid3 = fork();
  if(pid3 == 0) {
    worker("LOW", 20, duration);  // Lowest priority
    exit(0);
  }
  
  // 等待所有子进程完成
  for(int i = 0; i < 3; i++) {
    wait(0);
  }
  
  printf("\nAll processes completed!\n");
  
  exit(0);
}
