// bio.c - 块缓存管理 (Buffer Cache)
// 实验7：文件系统 - 块I/O子系统
#include "types.h"
#include "defs.h"
#include "fs.h"
#include "lock.h"
#include "string.h"
#include "printf.h"

// 块缓存结构定义
#define NBUF 30

static struct {
  spinlock lock;
  struct buf buf[NBUF];
} bcache;

// 磁盘I/O统计
static struct {
  uint64 reads;
  uint64 writes;
  uint64 cache_hits;
  uint64 cache_misses;
} disk_stats;

// 模拟磁盘存储
static uint8 disk[FSSIZE * BSIZE];

// ============ 底层磁盘I/O ============

void bread_sim(uint32 blockno, uint8 *data) {
  if (blockno >= FSSIZE) {
    printf("[bio] bread_sim: invalid blockno %d\n", blockno);
    return;
  }
  memcpy(data, &disk[blockno * BSIZE], BSIZE);
  disk_stats.reads++;
}

void bwrite_sim(uint32 blockno, uint8 *data) {
  if (blockno >= FSSIZE) {
    printf("[bio] bwrite_sim: invalid blockno %d\n", blockno);
    return;
  }
  memcpy(&disk[blockno * BSIZE], data, BSIZE);
  disk_stats.writes++;
}

// ============ 块缓存操作 ============

void binit(void) {
  initlock(&bcache.lock);
  
  for (int i = 0; i < NBUF; i++) {
    bcache.buf[i].valid = 0;
    bcache.buf[i].refcnt = 0;
  }
  
  disk_stats.reads = 0;
  disk_stats.writes = 0;
  disk_stats.cache_hits = 0;
  disk_stats.cache_misses = 0;
  
  printf("[bio] Block cache initialized (%d buffers)\n", NBUF);
}

struct buf* bread(uint32 blockno) {
  acquire(&bcache.lock);
  
  // 查找缓存
  for (int i = 0; i < NBUF; i++) {
    if (bcache.buf[i].valid && bcache.buf[i].blockno == blockno) {
      bcache.buf[i].refcnt++;
      disk_stats.cache_hits++;
      release(&bcache.lock);
      return &bcache.buf[i];
    }
  }
  
  disk_stats.cache_misses++;
  
  // 找空闲缓存
  for (int i = 0; i < NBUF; i++) {
    if (!bcache.buf[i].valid || bcache.buf[i].refcnt == 0) {
      // 如果有脏数据，先写回
      if (bcache.buf[i].valid && bcache.buf[i].dirty) {
        bwrite_sim(bcache.buf[i].blockno, bcache.buf[i].data);
        bcache.buf[i].dirty = 0;
      }
      
      bcache.buf[i].valid = 1;
      bcache.buf[i].blockno = blockno;
      bcache.buf[i].refcnt = 1;
      bcache.buf[i].dirty = 0;
      bread_sim(blockno, bcache.buf[i].data);
      release(&bcache.lock);
      return &bcache.buf[i];
    }
  }
  
  // 简单替换第一个（LRU替换策略的简化版）
  if (bcache.buf[0].dirty) {
    bwrite_sim(bcache.buf[0].blockno, bcache.buf[0].data);
  }
  bcache.buf[0].blockno = blockno;
  bcache.buf[0].refcnt = 1;
  bcache.buf[0].dirty = 0;
  bread_sim(blockno, bcache.buf[0].data);
  release(&bcache.lock);
  return &bcache.buf[0];
}

void bwrite(struct buf *b) {
  acquire(&bcache.lock);
  b->dirty = 1;
  bwrite_sim(b->blockno, b->data);
  release(&bcache.lock);
}

void brelse(struct buf *b) {
  acquire(&bcache.lock);
  if (b->refcnt > 0)
    b->refcnt--;
  release(&bcache.lock);
}

void bzero(int blockno) {
  struct buf *bp = bread(blockno);
  memset(bp->data, 0, BSIZE);
  bwrite(bp);
  brelse(bp);
}

void get_disk_stats(uint64 *reads, uint64 *writes, uint64 *hits, uint64 *misses) {
  *reads = disk_stats.reads;
  *writes = disk_stats.writes;
  *hits = disk_stats.cache_hits;
  *misses = disk_stats.cache_misses;
}
