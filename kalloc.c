#include "types.h"
#include "lock.h"
#include "riscv.h"
#include "memlayout.h"
#include "printf.h"
#include "string.h"

extern char end[];

typedef struct run {
  struct run *next;
} run;

static struct {
  spinlock lock;      
  run *freelist;
  uint64 free_pages;
} kmem;

void pmm_init_range(uint64 start, uint64 end);

// 初始化物理内存管理器
void pmm_init(void) {
  initlock(&kmem.lock);   // 初始化锁
  kmem.freelist = 0;        // 初始化空闲链表
  kmem.free_pages = 0;      // 初始化空闲页数

  uint64 pa_start = PGROUNDUP((uint64)end); // 物理内存起始地址
  uint64 pa_end   = PHYSTOP;                // 物理内存结束地址
  pmm_init_range(pa_start, pa_end);         // 初始化物理内存
  printf("pmm: init [%p, %p), free pages=%d\n", pa_start, pa_end, (int)kmem.free_pages);
}

// 释放一个物理页
static void kfree_nolock(void *pa) {
  run *r = (run*)pa;
  r->next = kmem.freelist;
  kmem.freelist = r;
  kmem.free_pages++;
}

// 释放一段物理内存
void pmm_init_range(uint64 start, uint64 end) {
  start = PGROUNDUP(start);
  for (uint64 p = start; p + PGSIZE <= end; p += PGSIZE) {
    memset((void*)p, 1, PGSIZE);  // 标记为可用
    kfree_nolock((void*)p);  // 加入空闲链表
  }
}

// 分配一个物理页
void* alloc_page(void) {
  acquire(&kmem.lock);    // 加锁
  run *r = kmem.freelist;  // 取空闲页
  if (r) {
    kmem.freelist = r->next;
    kmem.free_pages--;
  }
  release(&kmem.lock);  // 解锁

  if (r) memset((void*)r, 5, PGSIZE); // 将页填充5，标记为已分配
  return (void*)r;
}

// 释放一个物理页
void free_page(void *pa) {
  if (((uint64)pa % PGSIZE) != 0) {
    printf("free_page: not aligned %p\n", (uint64)pa);   // 非页对齐
    return;
  }
  if ((uint64)pa < PGROUNDUP((uint64)end) || (uint64)pa >= PHYSTOP) {
    printf("free_page: bad pa %p\n", (uint64)pa);   // 超出物理内存范围
    return;
  }
  memset(pa, 1, PGSIZE); // 标记为可用
  acquire(&kmem.lock);
  kfree_nolock(pa);
  release(&kmem.lock);
}

// 分配一段物理内存
void* alloc_pages(int n) {
  if (n <= 0) return 0;
  if (n == 1) return alloc_page();
  // 分配页表
  void *list[64];
  if (n > 64) return 0;
  for (int i = 0; i < n; i++) {
    list[i] = alloc_page();
    if (!list[i]) {
      for (int j = 0; j < i; j++) free_page(list[j]);
      return 0;
    }
  }
  return list[0];
}

// 释放一段物理内存
uint64 kfree_total(void) {
  acquire(&kmem.lock);
  uint64 n = kmem.free_pages;
  release(&kmem.lock);
  return n;
}
