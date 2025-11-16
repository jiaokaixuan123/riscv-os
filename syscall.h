#ifndef SYSCALL_H
#define SYSCALL_H
#define SYS_fork  1     // 创建进程
#define SYS_exit  2     // 退出进程
#define SYS_getpid 3    // 获取进程ID
#define SYS_yield 4     // 让出CPU
#define SYS_kill  5     // 终止进程
#define SYS_wait  6     // 等待进程结束
#define SYS_sleep 7     // 睡眠
#define SYS_setprio 8   // 设置优先级
#endif
