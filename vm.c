#include "types.h"
#include "riscv.h"
#include "kalloc.h"
#include "string.h"
#include "printf.h"
#include "memlayout.h"
#include "vm.h"
#include "uart.h" 

pagetable_t kernel_pagetable = 0;

// 创建一个新的页表
pagetable_t create_pagetable(void) {
  void *p = alloc_page(); // 申请一个页
  if (!p) return 0;
  memset(p, 0, PGSIZE); // 用0填充
  return (pagetable_t)p; // 转换为页表指针
}

// 遍历页表，查找va对应的pte，如果alloc为1，则分配一个新的pte
pte_t* walk(pagetable_t pt, uint64 va, int alloc) {
  if (va >= (1ULL<<39)) return 0; // 超过39位，不可能映射

  for (int level = 2; level > 0; level--) { // 遍历每一级
    pte_t *ptep = &pt[vpn(va, level)];
    pte_t pte = *ptep;          // 读取pte
    if (pte & PTE_V) {  // 如果pte有效
      if ((pte & (PTE_R|PTE_W|PTE_X)) == 0) {
        // 不是叶子PTE，则继续往下搜索
        pt = (pagetable_t)PTE_PA(pte);  // 转换为页表指针
      } else {
        return ptep;
      }
    } else {  // pte无效
      if (!alloc) return 0;  // 无效且不允许分配，返回0
      void *page = alloc_page();
      if (!page) return 0;  // 分配失败，返回0
      memset(page, 0, PGSIZE);  // 用0填充
      *ptep = PA2PTE(page) | PTE_V; // 非叶子PTE，分配一个页
      pt = (pagetable_t)page; // 转换为页表指针
    }
  }
  return &pt[vpn(va, 0)]; // 返回最后一级的pte指针
}

// 遍历页表，查找va对应的pte，如果没有找到，返回0
pte_t* walk_lookup(pagetable_t pt, uint64 va) {
  return walk(pt, va, 0);
}

// 遍历页表，查找va对应的pte，如果没有找到，分配一个新的pte
pte_t* walk_create(pagetable_t pt, uint64 va) {
  return walk(pt, va, 1);
}

// 将va到pa的sz大小的内存映射到pt中，perm为权限
int mappages(pagetable_t pt, uint64 va, uint64 pa, uint64 sz, int perm) {
  uint64 a = PGROUNDDOWN(va);  // 起始页对齐
  uint64 last = PGROUNDDOWN(va + sz - 1);  // 结束页对齐
  for (;;) {
    pte_t *ptep = walk(pt, a, 1);  // 找到或分配pte
    if (!ptep) goto bad;  // 分配失败，返回-1
    if (*ptep & PTE_V) goto bad;  // 已经映射，返回-1
    *ptep = PA2PTE(pa) | PTE_V | perm;  // 映射
    if (a == last) break;  // 映射完成
    a += PGSIZE;  // 继续映射下一页
    pa += PGSIZE;
  }
  return 0;
bad:
  uint64 b = PGROUNDDOWN(va);  // 起始页对齐
  while (b < a) {
    pte_t *q = walk(pt, b, 0);  // 找到pte
    if (q && (*q & PTE_V)) *q = 0;  // 清除pte
    b += PGSIZE;
  }
  return -1;
}

// 将va到pa的sz大小的内存映射到pt中，perm为权限
void map_region(pagetable_t pt, uint64 va, uint64 pa, uint64 sz, int perm) {
  if (mappages(pt, va, pa, sz, perm) < 0)
    printf("map_region: failed va=%p sz=%p\n", va, sz);
}

// 大小固定为PGSIZE
int map_page(pagetable_t pt, uint64 va, uint64 pa, int perm) {
  return mappages(pt, va, pa, PGSIZE, perm);
}

// 释放页表
static void freewalk(pagetable_t pt, int level) {
  for (int i = 0; i < NPTE; i++) {
    pte_t pte = pt[i];
    if ((pte & PTE_V) == 0) continue;
    if ((pte & (PTE_R|PTE_W|PTE_X)) == 0) {
      pagetable_t child = (pagetable_t)PTE_PA(pte);
      freewalk(child, level-1);
      pt[i] = 0;
      free_page((void*)child);
    } else {
      // leaf: free the mapped physical page to avoid leak
      void *leaf = (void*)PTE_PA(pte);
      pt[i] = 0;
      free_page(leaf);
    }
  }
}

// 释放页表
void destroy_pagetable(pagetable_t pt) {
  freewalk(pt, 2);
  free_page((void*)pt);
}

// 打印页表
void dump_pagetable_rec(pagetable_t pt, int level, uint64 va_prefix) {
  for (int i = 0; i < NPTE; i++) {
    pte_t pte = pt[i];
    if (!(pte & PTE_V)) continue;
    uint64 va_base = va_prefix | ((uint64)i << (PGSHIFT + 9*level)); // 计算va
    if ((pte & (PTE_R|PTE_W|PTE_X)) == 0) {  // 非叶子pte，递归打印
      dump_pagetable_rec((pagetable_t)PTE_PA(pte), level-1, va_base);
    } else {  // 叶子pte，直接打印
      uint64 pa = PTE_PA(pte);
      int r = !!(pte & PTE_R), w = !!(pte & PTE_W), x = !!(pte & PTE_X);  // r是1表示可读，w是1表示可写，x是1表示可执行
      int u = !!(pte & PTE_U);  // u是1表示用户态
      printf("[L%d] VA 0x%lx -> PA 0x%lx perm=%c%c%c U=%d\n", level, va_base, pa,
             r?'r':'-', w?'w':'-', x?'x':'-', u);
    }
  }
}

// 打印页表
void dump_pagetable(pagetable_t pt) { dump_pagetable_rec(pt, 2, 0); }

extern char etext[];   // 链接脚本里 PROVIDE(etext = .);

// 初始化内核页表
void kvminit(void) {
  kernel_pagetable = create_pagetable();

  // 1) 映射内核代码段 [KERNBASE, etext) -> RX
  uint64 text_start = KERNBASE;
  uint64 text_sz    = (uint64)etext - KERNBASE;
  map_region(kernel_pagetable,
             text_start, text_start,  
             PGROUNDUP(text_sz),              // <<<< 向上取整页
             PTE_R | PTE_X);

  // 2) 映射内核数据段及剩余物理内存 [PGROUNDUP(etext), PHYSTOP) -> RW
  uint64 data_start = PGROUNDUP((uint64)etext);  // <<<< 起始页对齐
  map_region(kernel_pagetable,
             data_start, data_start,
             PHYSTOP - data_start,             // 这里已经从页边界开始，可不再取整
             PTE_R | PTE_W);

  // 3) 映射设备（UART等）保持恒等映射
  map_region(kernel_pagetable, UART0, UART0, PGSIZE, PTE_R | PTE_W);
}

void kvminithart(void) {
  if (!kernel_pagetable) { printf("kvminithart: no kernel_pagetable\n"); return; }
  w_satp(MAKE_SATP(kernel_pagetable));
  sfence_vma();
}
