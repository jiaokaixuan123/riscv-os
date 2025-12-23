#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

// 测试基本系统调用功能
static void test_basic_syscalls(void) {
  printf("[syscall] basic tests...\n");
  int pid = getpid();
  printf("current pid=%d\n", pid);

  int child = fork();
  if (child == 0) {
    // child
    printf("child pid=%d\n", getpid());
    exit(42);
  } else if (child > 0) {
    int status = -1;
    wait(&status);
    printf("wait: child exit status=%d\n", status);
  } else {
    printf("fork failed\n");
  }
}

// 测试系统调用参数传递与边界情况
static void test_parameter_passing(void) {
  printf("[syscall] parameter passing...\n");
  char msg[] = "Hello, syscalls!\n";
  int n = write(1, msg, strlen(msg));       // write 系统调用
  printf("write stdout bytes=%d\n", n);

  // 空长度
  n = write(1, msg, 0);
  printf("write zero-len=%d\n", n);

  // 无效文件描述符
  n = write(-1, msg, strlen(msg));
  printf("write invalid fd ret=%d\n", n);

  // 文件操作
  int fd = open("README", O_RDONLY);
  if (fd >= 0) {
    char buf[32];
    int r = read(fd, buf, sizeof(buf));
    printf("read README bytes=%d\n", r);
    close(fd);
  } else {
    printf("open README failed (ok if file missing)\n");
  }
}

// 测试系统调用性能
static void test_syscall_performance(void) {
  printf("[syscall] performance...\n");
  // use uptime() ticks
  int t0 = uptime();
  for (int i = 0; i < 10000; i++) {
    (void)getpid();
  }
  int t1 = uptime();
  printf("10000 getpid() ticks delta=%d\n", t1 - t0);
}

int
main(int argc, char **argv)
{
  test_basic_syscalls();
  test_parameter_passing();
  test_syscall_performance();
  exit(0);
}
