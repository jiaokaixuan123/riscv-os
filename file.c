// file.c - 文件描述符管理 (File Descriptor Management)
#include "types.h"
#include "defs.h"
#include "fs.h"
#include "lock.h"
#include "printf.h"

// 文件表
static struct {
  spinlock lock;
  struct file file[NFILE];
} ftable;

void fileinit(void) {
  initlock(&ftable.lock);
  for (int i = 0; i < NFILE; i++) {
    ftable.file[i].ref = 0;
  }
  printf("[file] File table initialized (%d entries)\n", NFILE);
}

struct file* filealloc(void) {
  acquire(&ftable.lock);
  for (int i = 0; i < NFILE; i++) {
    if (ftable.file[i].ref == 0) {
      ftable.file[i].ref = 1;
      release(&ftable.lock);
      return &ftable.file[i];
    }
  }
  release(&ftable.lock);
  return 0;
}

struct file* filedup(struct file *f) {
  acquire(&ftable.lock);
  if (f->ref < 1) {
    release(&ftable.lock);
    return 0;
  }
  f->ref++;
  release(&ftable.lock);
  return f;
}

void fileclose(struct file *f) {
  acquire(&ftable.lock);
  if (f->ref < 1) {
    release(&ftable.lock);
    return;
  }
  if (--f->ref > 0) {
    release(&ftable.lock);
    return;
  }
  struct file ff = *f;
  f->ref = 0;
  f->type = FD_NONE;
  release(&ftable.lock);
  
  if (ff.type == FD_INODE) {
    begin_op();
    iput(ff.ip);
    end_op();
  }
}

int filestat(struct file *f, struct stat *st) {
  if (f->type == FD_INODE) {
    ilock(f->ip);
    stati(f->ip, st);
    iunlock(f->ip);
    return 0;
  }
  return -1;
}

int fileread(struct file *f, char *addr, int n) {
  if (f->readable == 0)
    return -1;
    
  if (f->type == FD_INODE) {
    ilock(f->ip);
    int r = readi(f->ip, addr, f->off, n);
    if (r > 0)
      f->off += r;
    iunlock(f->ip);
    return r;
  }
  return -1;
}

int filewrite(struct file *f, char *addr, int n) {
  if (f->writable == 0)
    return -1;
    
  if (f->type == FD_INODE) {
    int max = ((MAXOPBLOCKS-1-1-2) / 2) * BSIZE;
    int i = 0;
    while (i < n) {
      int n1 = n - i;
      if (n1 > max)
        n1 = max;
        
      begin_op();
      ilock(f->ip);
      int r = writei(f->ip, addr + i, f->off, n1);
      if (r > 0)
        f->off += r;
      iunlock(f->ip);
      end_op();
      
      if (r != n1)
        break;
      i += r;
    }
    return i == n ? n : -1;
  }
  return -1;
}
