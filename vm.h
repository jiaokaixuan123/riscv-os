#ifndef VM_H
#define VM_H

#include "types.h"

typedef uint64 pte_t;
typedef uint64* pagetable_t;   // 4k的页表包含512个pte页表条目

pagetable_t create_pagetable(void); // 创建一个新的页表
int  mappages(pagetable_t pt, uint64 va, uint64 pa, uint64 sz, int perm);   // 将虚拟地址va映射到物理地址pa，大小为sz，权限为perm
void map_region(pagetable_t pt, uint64 va, uint64 pa, uint64 sz, int perm);   // 将一个虚拟地址空间映射到物理地址空间
int map_page(pagetable_t pt, uint64 va, uint64 pa, int perm);
void destroy_pagetable(pagetable_t pt);   // 销毁一个页表

pte_t* walk(pagetable_t pt, uint64 va, int alloc);   // 遍历页表，查找va对应的pte，如果alloc为1，则分配一个新的pte
pte_t* walk_lookup(pagetable_t pt, uint64 va);  // 遍历页表，查找va对应的pte，如果没有找到，返回0
pte_t* walk_create(pagetable_t pt, uint64 va);  // 遍历页表，查找va对应的pte，如果没有找到，分配一个新的pte

void dump_pagetable(pagetable_t pt);    // 打印页表

void kvminit(void);  // 初始化内核页表
void kvminithart(void);  // 初始化当前cpu的页表

extern pagetable_t kernel_pagetable;   // 内核页表

int copyin(pagetable_t pt, void *dst, uint64 srcva, uint64 len);   // 从用户空间拷贝到内核
int copyout(pagetable_t pt, uint64 dstva, void *src, uint64 len);  // 从内核拷贝到用户空间

#endif
