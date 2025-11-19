// log.c - 日志系统 (Write-Ahead Logging)
#include "types.h"
#include "defs.h"
#include "fs.h"
#include "lock.h"
#include "string.h"
#include "printf.h"

static struct {
  spinlock lock;
  int start;
  int size;
  int outstanding;
  int committing;
  int dev;
  struct logheader lh;
} log;

static void log_write_header(void) {
  struct buf *buf = bread(log.start);
  struct logheader *hb = (struct logheader*)buf->data;
  hb->n = log.lh.n;
  for (int i = 0; i < log.lh.n; i++) {
    hb->block[i] = log.lh.block[i];
  }
  bwrite(buf);
  brelse(buf);
}

static void log_read_header(void) {
  struct buf *buf = bread(log.start);
  struct logheader *lh = (struct logheader*)buf->data;
  log.lh.n = lh->n;
  for (int i = 0; i < log.lh.n; i++) {
    log.lh.block[i] = lh->block[i];
  }
  brelse(buf);
}

static void log_install_trans(void) {
  for (int tail = 0; tail < log.lh.n; tail++) {
    struct buf *lbuf = bread(log.start + tail + 1);
    struct buf *dbuf = bread(log.lh.block[tail]);
    memcpy(dbuf->data, lbuf->data, BSIZE);
    bwrite(dbuf);
    brelse(lbuf);
    brelse(dbuf);
  }
}

static void log_recover(void) {
  log_read_header();
  log_install_trans();
  log.lh.n = 0;
  log_write_header();
}

static void log_commit(void) {
  if (log.lh.n > 0) {
    log_write_header();
    log_install_trans();
    log.lh.n = 0;
    log_write_header();
  }
}

void log_init(int dev, struct superblock *sb) {
  initlock(&log.lock);
  log.start = sb->logstart;
  log.size = sb->nlog;
  log.dev = dev;
  log.outstanding = 0;
  log.committing = 0;
  log.lh.n = 0;
  log_recover();
  printf("[log] Log system initialized (start=%d size=%d)\n", log.start, log.size);
}

void begin_op(void) {
  acquire(&log.lock);
  while(1) {
    if (log.committing) {
      release(&log.lock);
      for (volatile int i = 0; i < 1000; i++);
      acquire(&log.lock);
    } else if (log.lh.n + (log.outstanding+1)*MAXOPBLOCKS > LOGSIZE) {
      release(&log.lock);
      for (volatile int i = 0; i < 1000; i++);
      acquire(&log.lock);
    } else {
      log.outstanding += 1;
      release(&log.lock);
      break;
    }
  }
}

void end_op(void) {
  int do_commit = 0;
  acquire(&log.lock);
  log.outstanding -= 1;
  if (log.committing) {
    panic("log.committing");
  }
  if (log.outstanding == 0) {
    do_commit = 1;
    log.committing = 1;
  }
  release(&log.lock);
  if (do_commit) {
    log_commit();
    acquire(&log.lock);
    log.committing = 0;
    release(&log.lock);
  }
}

void log_write_block(struct buf *b) {
  acquire(&log.lock);
  if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1) {
    panic("log too big");
  }
  if (log.outstanding < 1) {
    panic("log_write outside of trans");
  }
  int i;
  for (i = 0; i < log.lh.n; i++) {
    if (log.lh.block[i] == (int)b->blockno)
      break;
  }
  log.lh.block[i] = b->blockno;
  if (i == log.lh.n) {
    log.lh.n++;
  }
  struct buf *lbuf = bread(log.start + i + 1);
  memcpy(lbuf->data, b->data, BSIZE);
  bwrite(lbuf);
  brelse(lbuf);
  release(&log.lock);
}
