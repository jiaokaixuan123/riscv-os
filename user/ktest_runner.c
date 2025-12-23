#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char **argv)
{
  printf("[USER] 调用 ktest() 触发内核测试...\n");
  ktest();
  printf("[USER] 内核测试触发完成\n");
  exit(0);
}
