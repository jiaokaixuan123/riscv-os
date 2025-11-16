// riscv.h - RISC-V 寄存器和页字段
#ifndef RISCV_H
#define RISCV_H

#include "types.h"

// 分页与地址转换
#define MAXVA (1L << (9 + 9 + 9 + 12 - 1))  // 最大虚拟地址

#ifndef __ASSEMBLER__

// 分页与地址转换
static inline void w_satp(uint64 x) {     // 写satp寄存器
  asm volatile("csrw satp, %0" :: "r"(x));
}
static inline uint64 r_satp(void) {       // 读satp寄存器
  uint64 x; asm volatile("csrr %0, satp" : "=r"(x));
  return x;
}
static inline void sfence_vma(void) {    // 刷新TLB
  asm volatile("sfence.vma zero, zero");
}

// Paging constants
#define PGSIZE      4096UL
#define PGSHIFT     12
#define NPTE        512        // 4KiB / 8B

#define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))  //宏定义：往上4k地址对齐
#define PGROUNDDOWN(a) ((a) & ~(PGSIZE-1))             //宏定义：往下4k地址对齐

// PTE bits (match RISC-V Sv39)
#define PTE_V (1L<<0)   // Valid
#define PTE_R (1L<<1)   // Read
#define PTE_W (1L<<2)   // Write
#define PTE_X (1L<<3)   // Execute
#define PTE_U (1L<<4)   // User
#define PTE_G (1L<<5)   // Global
#define PTE_A (1L<<6)   // Accessed
#define PTE_D (1L<<7)   // Dirty

#define PTE_PA(pte)    (((pte) >> 10) << 12)  // 映射页的物理基地址（低12位为0）
#define PA2PTE(pa)     ((((uint64)(pa)) >> 12) << 10) // 把物理地址pa放进PPN字段

// satp format: MODE[63:60]=8 for Sv39, ASID[59:44], PPN[43:0]
#define SATP_MODE_SV39  (8ULL << 60)  // satp寄存器中Sv39模式标志
#define MAKE_SATP(root_pa) ( SATP_MODE_SV39 | ( ((uint64)(root_pa) >> 12) & ((1ULL<<44)-1) ) )  // 构造satp值 Mode = 8, ASID=0, PPN=根页表物理地址>>12

// VPN 取虚拟地址va指定层级的9位VPN索引
static inline int vpn(uint64 va, int level) { // level = 2,1,0
  return (va >> (PGSHIFT + 9*level)) & 0x1FF; // 返回第level级页表项的第va位
}

// Supervisor级陷阱/中断 CSR
static inline uint64 r_sstatus() { uint64 x; asm volatile("csrr %0, sstatus" : "=r"(x)); return x; }  // 读sstatus寄存器（含SIE/SPIE/SPP）
static inline void   w_sstatus(uint64 x){ asm volatile("csrw sstatus, %0" :: "r"(x)); }
static inline uint64 r_scause()  { uint64 x; asm volatile("csrr %0, scause"  : "=r"(x)); return x; }  // 读scause寄存器（陷阱原因，最高位 = 中断标志，低位 = 原因码）
static inline uint64 r_sepc()    { uint64 x; asm volatile("csrr %0, sepc"    : "=r"(x)); return x; }  // 读sepc寄存器（陷阱返回地址：异常指令地址或下一条指令地址）
static inline void   w_sepc(uint64 x)   { asm volatile("csrw sepc, %0" :: "r"(x)); }
static inline void   w_stvec(uint64 x)  { asm volatile("csrw stvec, %0" :: "r"(x)); }
static inline uint64 r_stvec()   { uint64 x; asm volatile("csrr %0, stvec"   : "=r"(x)); return x; }  // 读stvec寄存器（陷阱向量入口地址）
static inline uint64 r_sie()     { uint64 x; asm volatile("csrr %0, sie"     : "=r"(x)); return x; }  // 读sie寄存器（中断使能位集合 STIE/SEIE等）
static inline void   w_sie(uint64 x)    { asm volatile("csrw sie, %0" :: "r"(x)); } 
static inline uint64 r_stval(void){ uint64 x; asm volatile("csrr %0, stval":"=r"(x)); return x; }     // 读stval寄存器（异常相关地址或指令）

// sstatus bits
#define SSTATUS_SPP (1L << 8)   // sstatus中保存的先前特权级 ： 1 = supervisor ，0 = User
#define SSTATUS_SPIE (1L << 5)  // 上一次陷入前的S模式中断使能保存位，sretore时恢复SIE
#define SSTATUS_SIE (1L << 1)   // 当前S模式中断使能位 1= 允许中断，0=禁止中断

// sie bits
#define SIE_STIE (1L << 5)  // sie中的定时器中断使能位
#define SIE_SEIE (1L << 9)  // sie中的外部设备中断使能位

// 时间与定时器
static inline uint64 r_time() { uint64 x; asm volatile("csrr %0, time" : "=r"(x)); return x; }  // 读time寄存器
static inline void   w_stimecmp(uint64 x){ asm volatile("csrw stimecmp, %0" :: "r"(x)); } // 写stimecmp寄存器,设置下一次S模式定时器中断时间

// 中断总控封装
static inline void intr_on()  { w_sstatus(r_sstatus() | SSTATUS_SIE); }   // 设置 sstatus.SIE = 1 ,开启中断
static inline void intr_off() { w_sstatus(r_sstatus() & ~SSTATUS_SIE); }  // 设置 sstatus.SIE = 0 ,关闭中断
static inline int  intr_get() { return (r_sstatus() & SSTATUS_SIE) != 0; }  // 获取当前中断状态

// Machine模式初始化相关
static inline uint64 r_mstatus(){ uint64 x; asm volatile("csrr %0, mstatus" : "=r"(x)); return x; }       // 读mstatus寄存器(含MPP等，用于模式切换)
static inline void   w_mstatus(uint64 x){ asm volatile("csrw mstatus, %0" :: "r"(x)); }
static inline void   w_mepc(uint64 x){ asm volatile("csrw mepc, %0" :: "r"(x)); }                         // 写mepc寄存器(设置mret返回地址) 
static inline void   w_medeleg(uint64 x){ asm volatile("csrw medeleg, %0" :: "r"(x)); }                   // 写medeleg寄存器(异常委托)
static inline void   w_mideleg(uint64 x){ asm volatile("csrw mideleg, %0" :: "r"(x)); }                   // 写mideleg寄存器(中断委托)
static inline uint64 r_mie(){ uint64 x; asm volatile("csrr %0, mie" : "=r"(x)); return x; }               // 读mie寄存器(中断使能位集合)
static inline void   w_mie(uint64 x){ asm volatile("csrw mie, %0" :: "r"(x)); }
static inline uint64 r_mhartid(){ uint64 x; asm volatile("csrr %0, mhartid" : "=r"(x)); return x; }       // 读mhartid寄存器(硬件线程ID)
static inline void   w_pmpaddr0(uint64 x){ asm volatile("csrw pmpaddr0, %0" :: "r"(x)); }                 // 写pmpaddr0寄存器(物理内存保护地址)
static inline void   w_pmpcfg0(uint64 x){ asm volatile("csrw pmpcfg0, %0" :: "r"(x)); }                   // 写pmpcfg0寄存器(物理内存保护配置)
static inline uint64 r_menvcfg(){ uint64 x; asm volatile("csrr %0, menvcfg" : "=r"(x)); return x; }       // 读menvcfg寄存器(环境配置)
static inline void   w_menvcfg(uint64 x){ asm volatile("csrw menvcfg, %0" :: "r"(x)); } 
static inline uint64 r_mcounteren(){ uint64 x; asm volatile("csrr %0, mcounteren" : "=r"(x)); return x; } // 读mcounteren寄存器(允许S模式访问计数器)
static inline void   w_mcounteren(uint64 x){ asm volatile("csrw mcounteren, %0" :: "r"(x)); }             // 允许S态访问计数器
static inline void w_mtvec(uint64 x) { asm volatile("csrw mtvec, %0" :: "r"(x)); }                        // 写mtvec寄存器(设置机器模式陷阱向量地址)
static inline uint64 r_mtvec(void) { uint64 x; asm volatile("csrr %0, mtvec" : "=r"(x)); return x; }

// mstatus bits
#define MSTATUS_MPP_MASK (3L << 11)   // MPP字段掩码,记录上次特权级
#define MSTATUS_MPP_S    (1L << 11)   // 设置MPP为S态

// mie bits
#define MIE_STIE (1L << 5)   // mie中的定时器中断使能位
static inline void w_tp(uint64 x){ asm volatile("mv tp, %0" :: "r"(x)); } // 写tp寄存器(线程指针)

#endif /* __ASSEMBLER__ */

#endif // RISCV_H
