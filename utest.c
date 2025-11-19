// 用户态系统调用测试程序
#include "types.h"
#include "usyscall.h"

// 注意：这些函数需要被编译成位置无关的代码

// 简单的用户态printf（通过内核的console输出）
// 由于用户态没有直接访问UART的权限，这里我们使用一个trick
// 实际上应该通过系统调用实现，但为了简化测试，我们先用内联汇编
static void uprint(const char *s) {
    // 临时方案：直接写UART（仅用于测试）
    // 生产环境应该使用 write() 系统调用
    volatile char *uart = (volatile char*)0x10000000;
    while (*s) {
        *uart = *s++;
    }
}

static void uprint_num(int n) {
    char buf[20];
    int i = 0;
    int neg = 0;
    
    if (n < 0) {
        neg = 1;
        n = -n;
    }
    
    if (n == 0) {
        buf[i++] = '0';
    } else {
        while (n > 0) {
            buf[i++] = '0' + (n % 10);
            n /= 10;
        }
    }
    
    if (neg) {
        buf[i++] = '-';
    }
    
    // 反转
    for (int j = 0; j < i / 2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = tmp;
    }
    buf[i] = '\0';
    
    uprint(buf);
}

// 测试 getpid 系统调用
void test_getpid(void) {
    int pid = getpid();
    uprint("[user] getpid returned: ");
    uprint_num(pid);
    uprint("\n");
}

// 测试 fork 系统调用
void test_fork(void) {
    uprint("[user] testing fork...\n");
    int pid = fork();
    
    if (pid == 0) {
        // 子进程
        int mypid = getpid();
        uprint("[user] child process, pid=");
        uprint_num(mypid);
        uprint("\n");
        exit(0);
    } else if (pid > 0) {
        // 父进程
        uprint("[user] parent process, child pid=");
        uprint_num(pid);
        uprint("\n");
        int status;
        wait(&status);
        uprint("[user] child exited\n");
    } else {
        uprint("[user] fork failed\n");
    }
}

// 测试 yield 系统调用
void test_yield(void) {
    uprint("[user] testing yield...\n");
    for (int i = 0; i < 3; i++) {
        uprint("[user] before yield ");
        uprint_num(i);
        uprint("\n");
        yield_user();
        uprint("[user] after yield ");
        uprint_num(i);
        uprint("\n");
    }
}

// 测试 sleep 系统调用
void test_sleep(void) {
    uprint("[user] testing sleep...\n");
    uprint("[user] sleeping for 10 ticks...\n");
    sleep_user(10);
    uprint("[user] woke up after sleep\n");
}

// 用户程序主入口
void user_main(void) {
    uprint("\n=== User Program Started ===\n");
    
    // 测试 getpid
    test_getpid();
    
    // 测试 yield
    test_yield();
    
    // 测试 sleep
    test_sleep();
    
    // 测试 fork
    test_fork();
    
    uprint("=== User Program Completed ===\n");
    exit(0);
}
