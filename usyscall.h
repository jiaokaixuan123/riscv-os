// 用户态系统调用接口
#ifndef USYSCALL_H
#define USYSCALL_H

#include "types.h"

// 系统调用声明
int fork(void);                     // 创建子进程
void exit(int status);              // 退出进程
int getpid(void);                   // 获取进程ID
void yield_user(void);              // 让出CPU（用户态版本）
int kill(int pid);                  // 终止进程
int wait(int *status);              // 等待子进程
int sleep_user(uint64 ticks);       // 睡眠指定ticks（用户态版本）
int setprio(int pid, int priority); // 设置优先级

#endif
