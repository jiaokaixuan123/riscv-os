#ifndef MEMLAYOUT_H
#define MEMLAYOUT_H

#define PGSIZE      4096UL
#define MAXVA (1L << (9 + 9 + 9 + 12 - 1))  // 最大虚拟地址

// QEMU virt: RAM typically starts at 0x80000000
#define KERNBASE   0x80000000UL

// Limit physical memory used by the kernel/allocator (128 MiB like xv6)
#define PHYSTOP    0x88000000UL

// Device MMIO base addresses (UART is already defined in uart.h)
#define CLINT      0x02000000UL
#define PLIC       0x0c000000UL

#define USER_TEXT_START  0x10000UL      // 简单用户代码起始VA
#define USER_STACK_TOP   0x80000000UL   // 用户栈顶VA
#define USER_STACK_SIZE  PGSIZE         // 用户栈大小一页
#define USER_MAX_VA      0x80000000UL   // 用户空间最大虚拟地址

// User memory layout.
// Address zero first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
//   ...
//   TRAPFRAME (p->trapframe, used by the trampoline)
//   TRAMPOLINE (the same page as in the kernel)
#define TRAMPOLINE (MAXVA - PGSIZE)
#define TRAPFRAME (TRAMPOLINE - PGSIZE)

#endif // MEMLAYOUT_H
