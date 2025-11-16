#include "types.h"
#include "vm.h"
#include "riscv.h"
#include "kalloc.h"
#include "printf.h"
#include <string.h>

// 复制用户空间到内核空间，pagetable为用户页表，dst为内核缓冲区，srcva为用户虚拟地址，len为长度
static int copy_one(pagetable_t pt, uint64 va, void *buf, uint64 off, uint64 n, int to_user){
  pte_t *pte = walk_lookup(pt, va);                           // 查找va对应的pte
  if(!pte || !(*pte & PTE_V) || !(*pte & PTE_U)) return -1;   // 无效或不可用,返回错误
  uint64 pa = PTE_PA(*pte);                                   // 获取物理地址
  uint64 pageoff = va & (PGSIZE-1);                           // 页内偏移
  uint64 remain = PGSIZE - pageoff;                           // 当前页剩余空间
  if(n > remain) n = remain;                                  // 限制复制长度不超过当前页
  if(to_user){
    // 内核 -> 用户
    memcpy((void*)(pa + pageoff), (char*)buf + off, n);
  } else {
    // 用户 -> 内核
    memcpy((char*)buf + off, (void*)(pa + pageoff), n);
  }
  return n;
}

// 复制用户空间到内核空间，pagetable为用户页表，dst为内核缓冲区，srcva为用户
int copyin(pagetable_t pt, void *dst, uint64 srcva, uint64 len){
  uint64 done = 0;
  while(done < len){
    int r = copy_one(pt, srcva + done, dst, done, len - done, 0);
    if(r < 0) return -1;
    done += r;
  }
  return 0;
}

// 复制内核空间到用户空间，pagetable为用户页表，dstva为用户虚拟地址，src为内核缓冲区，len为长度
int copyout(pagetable_t pt, uint64 dstva, void *src, uint64 len){
  uint64 done = 0;
  while(done < len){
    int r = copy_one(pt, dstva + done, src, done, len - done, 1);
    if(r < 0) return -1;
    done += r;
  }
  return 0;
}
