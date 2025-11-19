# 用户态系统调用汇编实现
# 所有系统调用通过 ecall 指令陷入内核

.section .text

# 系统调用通用包装
# 调用约定：系统调用号在 a7，参数在 a0-a5，返回值在 a0
.globl syscall_wrapper
syscall_wrapper:
    ecall           # 触发系统调用
    ret             # 返回，a0 包含返回值

# fork() - 创建子进程
.globl fork
fork:
    li a7, 1        # SYS_fork = 1
    ecall
    ret

# exit(status) - 退出进程
.globl exit
exit:
    li a7, 2        # SYS_exit = 2
    ecall
    ret

# getpid() - 获取进程ID
.globl getpid
getpid:
    li a7, 3        # SYS_getpid = 3
    ecall
    ret

# yield() - 让出CPU
.globl yield_user
yield_user:
    li a7, 4        # SYS_yield = 4
    ecall
    ret

# kill(pid) - 终止进程
.globl kill
kill:
    li a7, 5        # SYS_kill = 5
    ecall
    ret

# wait(status) - 等待子进程
.globl wait
wait:
    li a7, 6        # SYS_wait = 6
    ecall
    ret

# sleep(ticks) - 睡眠指定ticks
.globl sleep_user
sleep_user:
    li a7, 7        # SYS_sleep = 7
    ecall
    ret

# setprio(pid, priority) - 设置优先级
.globl setprio
setprio:
    li a7, 8        # SYS_setprio = 8
    ecall
    ret
