#ifndef TRAPFRAME_H
#define TRAPFRAME_H
#include "types.h"
// 与 trampoline.S 对齐的陷阱帧布局
// 顺序与偏移必须匹配：kernel_satp, kernel_sp, kernel_trap, user epc, kernel_hartid, 后续通用寄存器
struct trapframe {
  uint64 kernel_satp;    // 0  : 内核页表 (satp)
  uint64 kernel_sp;      // 8  : 进程内核栈顶
  uint64 kernel_trap;    // 16 : usertrap() 地址
  uint64 sepc;           // 24 : 用户态 PC (epc)
  uint64 kernel_hartid;  // 32 : 保存tp
  uint64 ra;             // 40
  uint64 sp;             // 48
  uint64 gp;             // 56
  uint64 tp;             // 64
  uint64 t0;             // 72
  uint64 t1;             // 80
  uint64 t2;             // 88
  uint64 s0;             // 96
  uint64 s1;             // 104
  uint64 a0;             // 112
  uint64 a1;             // 120
  uint64 a2;             // 128
  uint64 a3;             // 136
  uint64 a4;             // 144
  uint64 a5;             // 152
  uint64 a6;             // 160
  uint64 a7;             // 168
  uint64 s2;             // 176
  uint64 s3;             // 184
  uint64 s4;             // 192
  uint64 s5;             // 200
  uint64 s6;             // 208
  uint64 s7;             // 216
  uint64 s8;             // 224
  uint64 s9;             // 232
  uint64 s10;            // 240
  uint64 s11;            // 248
  uint64 t3;             // 256
  uint64 t4;             // 264
  uint64 t5;             // 272
  uint64 t6;             // 280
  // 额外保存CSR信息（不是 trampoline 固定偏移需要，可选）：
  uint64 sstatus;        // 保存进入前 sstatus
  uint64 scause;         // 保存 scause
};
#endif // TRAPFRAME_H
