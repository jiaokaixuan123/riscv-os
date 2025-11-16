#ifndef INTERRUPT_H
#define INTERRUPT_H
#include "types.h"

typedef void (*interrupt_handler_t)(void);  // 中断处理函数类型

void trap_init(void);                // 初始化中断系统
void register_interrupt(int irq, interrupt_handler_t h); // 注册中断处理函数
void enable_interrupt(int irq);      // 使能指定IRQ（目前支持timer与外部）
void disable_interrupt(int irq);     // 关闭指定IRQ
uint64 get_time(void);               // 读取当前时间CSR

extern volatile uint64 ticks;        // 时钟tick计数
extern volatile uint64 timer_interrupts; // 时钟中断次数

#endif // INTERRUPT_H
