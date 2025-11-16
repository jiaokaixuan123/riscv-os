
// lock.h - very small spinlock for uniprocessor / basic SMP
#ifndef LOCK_H
#define LOCK_H

#include "types.h"

// 自旋锁
typedef struct {
  volatile uint locked;
} spinlock;

// 初始化锁
static inline void initlock(spinlock *lk) {
  lk->locked = 0;
}

// 获取锁
static inline void acquire(spinlock *lk) {
  while(__sync_lock_test_and_set(&lk->locked, 1) != 0) { }  // 自旋等待
  __sync_synchronize(); // 内存屏障，防止编译器重排序
}

// 释放锁
static inline void release(spinlock *lk) {
  __sync_synchronize();
  __sync_lock_release(&lk->locked);
}

#endif // LOCK_H
