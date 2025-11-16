#include "types.h"
#include "proc.h"
#include <stddef.h>
// 导出 trapframe 指针字段在 struct proc 中的偏移供汇编使用
const uint64 PROC_TRAPFRAME_OFF = (uint64)offsetof(struct proc, trapframe);
