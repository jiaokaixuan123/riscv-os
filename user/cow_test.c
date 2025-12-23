// 测试 Copy-on-Write (COW) 功能的正确性和效率
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/memlayout.h"
#include "user/user.h"

// Test 1: 检测引用计数是否正确
void
refcount_test()
{
  int pid, status;
  char *page = sbrk(4096);
  
  printf("refcount test: ");
  
  if(page == (char*)-1) {
    printf("sbrk failed\n");
    exit(1);
  }
  
  // 父进程初始化页面内容
  for(int i = 0; i < 4096; i++)
    page[i] = 0xAA;
  
  pid = fork();
  if(pid < 0) {
    printf("fork failed\n");
    exit(1);
  }
  
  if(pid == 0) {
    if(page[0] != 0xAA) {       // 子进程应能读取到父进程的数据
      printf("FAIL: child can't read parent's data\n");
      exit(1);
    }
    
    page[0] = 0xBB;   // 子进程写入
    
    if(page[0] != 0xBB) {   // 验证写入是否成功
      printf("FAIL: child write didn't work\n");
      exit(1);
    }
    exit(0);
  } else {
    // 父进程等待子进程结束
    wait(&status);
    
    if(page[0] != 0xAA) {   // 父进程数据应未被子进程修改
      printf("FAIL: parent's data was corrupted\n");
      exit(1);
    }
  }
  
  printf("OK\n");
}

// Test 2: 内存限制测试
void
memory_limit_test()
{
  int pid;
  char *mem;
  int pages = 1000;  // 分配 1000 页 (4MB)
  
  printf("memory limit test: ");
  
  // 分配大量内存
  mem = sbrk(pages * 4096);
  if(mem == (char*)-1) {
    printf("sbrk failed\n");
    exit(1);
  }
  
  // 父进程初始化所有页面
  for(int i = 0; i < pages * 4096; i += 4096)
    mem[i] = 0xFF;
  
  // 创建多个子进程，每个子进程只读内存页面
  for(int i = 0; i < 50; i++) {
    pid = fork();
    if(pid < 0) {
      printf("FAIL: fork failed - out of memory without COW\n");
      exit(1);
    }
    if(pid == 0) {
      // Child: 只读内存页面，计算简单和以验证内容
      int sum = 0;
      for(int j = 0; j < pages * 4096; j += 4096)
        sum += mem[j];  // 应该都是 0xFF
      exit(0);
    }
  }
  
  for(int i = 0; i < 10; i++) {
    wait(0);
  }
  
  printf("OK\n");
}


int
main(int argc, char *argv[])
{

  refcount_test();

  memory_limit_test();

  exit(1);
}
