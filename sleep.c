#include "types.h"
#include "lock.h"
#include "riscv.h"
#include "defs.h"
#include "printf.h"

// 简易 sleep: 忙等直到 ticks 达到 target_ticks。
// 后续可替换为阻塞队列/调度。
void sleep(uint64 target_ticks){
  while(get_ticks() < target_ticks){
    // 允许中断，使定时器推进
    asm volatile("wfi"); // 等待中断 (可选)
  }
}
