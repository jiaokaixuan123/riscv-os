// init: 初始化用户空间，并启动第一个 shell 进程

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/fs.h"
#include "kernel/file.h"
#include "user/user.h"
#include "kernel/fcntl.h"

char *argv[] = { "sh", 0 };

int
main(void)
{
  int pid, wpid;

  if(open("console", O_RDWR) < 0){      // 打开控制台设备
    mknod("console", CONSOLE, 0);       // 创建设备节点
    open("console", O_RDWR);          // 再次打开控制台设备
  }
  dup(0);  // stdout
  dup(0);  // stderr

  for(;;){
    printf("init: starting sh\n");
    pid = fork();     // 创建子进程
    if(pid < 0){
      printf("init: fork failed\n");
      exit(1);
    }
    if(pid == 0){
      exec("sh", argv);
      printf("init: exec sh failed\n");
      exit(1);
    }

    for(;;){
      wpid = wait((int *) 0);   // 等待子进程终止
      if(wpid == pid){
        // 子进程已终止，重新启动 shell
        break;
      } else if(wpid < 0){
        printf("init: wait returned an error\n");
        exit(1);
      } else {
        // 其他子进程终止，继续等待
      }
    }
  }
}
