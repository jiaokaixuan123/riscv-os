#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

// 创建目录 dd，切换到该目录，删除上级目录中的 dd，然后等待被杀死以测试孤儿进程处理。

#define BUFSZ 500

char buf[BUFSZ];

int
main(int argc, char **argv)
{
  char *s = argv[0];

  if(mkdir("dd") != 0){   // 创建目录 dd
    printf("%s: mkdir dd failed\n", s);
    exit(1);
  }

  if(chdir("dd") != 0){   // 切换到目录 dd
    printf("%s: chdir dd failed\n", s);
    exit(1);
  }

  if (unlink("../dd") < 0) {    // 删除上级目录中的 dd
    printf("%s: unlink failed\n", s);
    exit(1);
  }
  printf("wait for kill and reclaim\n");
  // sit around until killed
  for(;;) pause(1000);
}
