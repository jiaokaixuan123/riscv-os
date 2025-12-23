#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

// logstress.c: 测试日志系统的压力测试程序

#define BUFSZ 500

char buf[BUFSZ];

int
main(int argc, char **argv)
{
  int fd, n;
  enum { N = 250, SZ=2000 };        // 每个子进程写 N 次，每次 SZ 字节
  
  for (int i = 1; i < argc; i++){   // 为每个文件名创建一个子进程
    int pid1 = fork();
    if(pid1 < 0){
      printf("%s: fork failed\n", argv[0]);
      exit(1);
    }
    if(pid1 == 0) {
      fd = open(argv[i], O_CREATE | O_RDWR);    // 创建并打开文件
      if(fd < 0){
        printf("%s: create %s failed\n", argv[0], argv[i]);
        exit(1);
      }
      memset(buf, '0'+i, SZ);                   // 用不同字符填充缓冲区
      for(i = 0; i < N; i++){            // 写入 N 次
        if((n = write(fd, buf, SZ)) != SZ){     // 写入 SZ 字节
          printf("write failed %d\n", n);
          exit(1);
        }
      }
      exit(0);
    }
  }
  int xstatus;
  for(int i = 1; i < argc; i++){
    wait(&xstatus);
    if(xstatus != 0)
      exit(xstatus);
  }
  return 0;
}
