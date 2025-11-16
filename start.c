#include "types.h"
#include "riscv.h"
#include "defs.h"

extern void main();

// 定时器初始化，请求每个 hart 产生定时器中断
void
timerinit()
{
  // Supervisor 模式启用定时器中断
  w_mie(r_mie() | MIE_STIE);
  
  // 启用 sstc 扩展（即 stimecmp）
  w_menvcfg(r_menvcfg() | (1L << 63)); 
  
  // 允许 S 模式访问计数器 (stime/cycle 等)
  w_mcounteren(r_mcounteren() | 2);
  
  // 设置下一次中断时间为当前时间+1秒 (可调)
  w_stimecmp(r_time() + 1000000);
}

// 在 M 模式启动，设置委托后切换到 S 模式
void start() {
  // 设置 M Previous Privilege mode 为 Supervisor
  uint64 x = r_mstatus();
  x &= ~MSTATUS_MPP_MASK;
  x |= MSTATUS_MPP_S;
  w_mstatus(x);               

  // 设置 M Exception Program Counter 为 main
  w_mepc((uint64)main);

  // 禁用分页
  w_satp(0);

  // 委托异常与中断到 S 模式 (此处仍用全 0xffff，可根据需要精简)
  w_medeleg(0xffff);
  w_mideleg(0xffff);

  // 启用 S 模式中断 (允许外部和定时器)
  w_sie(r_sie() | SIE_SEIE | SIE_STIE);

  // 配置 PMP 允许 S 模式访问所有物理内存
  w_pmpaddr0(0x3fffffffffffffull);
  w_pmpcfg0(0xf);

  // 初始化定时器
  timerinit();

  // hartid 已在 entry.S 中写入 tp，不再在 S 模式读取 mhartid 避免非法指令
  // int id = r_mhartid();
  // w_tp(id);

  // 切换到 S 模式并跳转到 main
  asm volatile("mret");
}
