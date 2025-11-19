// fs.c - 文件系统核心实现 (Core File System)
// 实验7：inode管理、目录操作、路径解析
#include "types.h"
#include "defs.h"
#include "fs.h"
#include "printf.h"
#include "string.h"
#include "lock.h"

// Inode缓存
static struct {
  spinlock lock;
  struct inode inode[NINODES];
} icache;

// 超级块
static struct superblock sb;
// ============ 位图操作 ============

static uint32 balloc(uint32 dev) {
  struct buf *bp;
  (void)dev;  // 当前简化实现未使用设备号
  
  for (uint32 b = 0; b < sb.size; b += BSIZE*8) {
    bp = bread(sb.bmapstart + b/(BSIZE*8));
    for (int bi = 0; bi < BSIZE*8 && b + bi < sb.size; bi++) {
      int m = 1 << (bi % 8);
      if ((bp->data[bi/8] & m) == 0) {
        bp->data[bi/8] |= m;
        bwrite(bp);
        brelse(bp);
        bzero(b + bi);
        return b + bi;
      }
    }
    brelse(bp);
  }
  printf("[fs] balloc: out of blocks\n");
  return 0;
}

static void bfree(uint32 dev, uint32 b) {
  (void)dev;  // 当前简化实现未使用设备号
  struct buf *bp = bread(sb.bmapstart + b/(BSIZE*8));
  int bi = b % (BSIZE*8);
  int m = 1 << (bi % 8);
  bp->data[bi/8] &= ~m;
  bwrite(bp);
  brelse(bp);
}

// ============ Inode操作 ============
void iinit(void) {
  initlock(&icache.lock);
  
  for (int i = 0; i < NINODES; i++) {
    icache.inode[i].ref = 0;
  }
  printf("[fs] Inode cache initialized (%d inodes)\n", NINODES);
}

struct inode* iget(uint32 dev, uint32 inum) {
  acquire(&icache.lock);
  
  // 查找已存在的
  for (int i = 0; i < NINODES; i++) {
    if (icache.inode[i].ref > 0 && 
        icache.inode[i].dev == dev && 
        icache.inode[i].inum == inum) {
      icache.inode[i].ref++;
      release(&icache.lock);
      return &icache.inode[i];
    }
  }
  
  // 分配新的
  for (int i = 0; i < NINODES; i++) {
    if (icache.inode[i].ref == 0) {
      icache.inode[i].dev = dev;
      icache.inode[i].inum = inum;
      icache.inode[i].ref = 1;
      icache.inode[i].valid = 0;
      release(&icache.lock);
      return &icache.inode[i];
    }
  }
  
  release(&icache.lock);
  printf("[fs] iget: no inodes\n");
  return 0;
}

struct inode* idup(struct inode *ip) {
  acquire(&icache.lock);
  ip->ref++;
  release(&icache.lock);
  return ip;
}

void ilock(struct inode *ip) {
  if (ip == 0 || ip->ref < 1)
    return;
    
  if (ip->valid == 0) {
    struct buf *bp = bread(sb.inodestart + ip->inum/8);
    struct dinode *dip = (struct dinode*)bp->data + ip->inum%8;
    
    ip->type = dip->type;
    ip->major = dip->major;
    ip->minor = dip->minor;
    ip->nlink = dip->nlink;
    ip->size = dip->size;
    memcpy(ip->addrs, dip->addrs, sizeof(ip->addrs));
    ip->valid = 1;
    brelse(bp);  // 添加缺失的 brelse
  }
}

void iunlock(struct inode *ip) {
  // 当前实现中没有显式锁结构，保持占位
  (void)ip; // 避免未使用参数警告
}

void iput(struct inode *ip) {
  acquire(&icache.lock);
  if (ip->ref == 1 && ip->valid && ip->nlink == 0) {
    iunlock(ip);
    itrunc(ip);
    ip->type = 0;
    iupdate(ip);
    ip->valid = 0;
  }
  ip->ref--;
  release(&icache.lock);
}

void iunlockput(struct inode *ip) {
  iunlock(ip);
  iput(ip);
}

void iupdate(struct inode *ip) {
  struct buf *bp = bread(sb.inodestart + ip->inum/8);
  struct dinode *dip = (struct dinode*)bp->data + ip->inum%8;
  
  dip->type = ip->type;
  dip->major = ip->major;
  dip->minor = ip->minor;
  dip->nlink = ip->nlink;
  dip->size = ip->size;
  memcpy(dip->addrs, ip->addrs, sizeof(ip->addrs));
  
  bwrite(bp);
  brelse(bp);  // 添加缺失的 brelse
}

static uint32 bmap(struct inode *ip, uint32 bn) {
  uint32 addr;
  
  if (bn < NDIRECT) {
    if ((addr = ip->addrs[bn]) == 0)
      ip->addrs[bn] = addr = balloc(ip->dev);
    return addr;
  }
  bn -= NDIRECT;
  
  if (bn < NINDIRECT) {
    if ((addr = ip->addrs[NDIRECT]) == 0)
      ip->addrs[NDIRECT] = addr = balloc(ip->dev);
    struct buf *bp = bread(addr);
    uint32 *a = (uint32*)bp->data;
    if ((addr = a[bn]) == 0) {
      a[bn] = addr = balloc(ip->dev);
      bwrite(bp);
    }
    brelse(bp);  // 添加缺失的 brelse
    return addr;
  }
  
  return 0;
}

void itrunc(struct inode *ip) {
  for (int i = 0; i < NDIRECT; i++) {
    if (ip->addrs[i]) {
      bfree(ip->dev, ip->addrs[i]);
      ip->addrs[i] = 0;
    }
  }
  
  if (ip->addrs[NDIRECT]) {
    struct buf *bp = bread(ip->addrs[NDIRECT]);
    uint32 *a = (uint32*)bp->data;
    for (uint32 j = 0; j < NINDIRECT; j++) {  // 使用 uint32 避免符号比较警告
      if (a[j])
        bfree(ip->dev, a[j]);
    }
    brelse(bp);  // 添加缺失的 brelse
    bfree(ip->dev, ip->addrs[NDIRECT]);
    ip->addrs[NDIRECT] = 0;
  }
  
  ip->size = 0;
  iupdate(ip);
}

struct inode* ialloc(uint32 dev, short type) {
  for (uint32 inum = 1; inum < sb.ninodes; inum++) {
    struct buf *bp = bread(sb.inodestart + inum/8);
    struct dinode *dip = (struct dinode*)bp->data + inum%8;
    
    if (dip->type == 0) {
      memset(dip, 0, sizeof(*dip));
      dip->type = type;
      bwrite(bp);
      brelse(bp);  // 添加缺失的 brelse
      return iget(dev, inum);
    }
    brelse(bp);  // 添加缺失的 brelse
  }
  return 0;
}

int readi(struct inode *ip, char *dst, uint32 off, uint32 n) {
  if (off > ip->size || off + n < off)
    return 0;
  if (off + n > ip->size)
    n = ip->size - off;
    
  uint32 tot, m;
  for (tot=0; tot<n; tot+=m, off+=m, dst+=m) {
    uint32 addr = bmap(ip, off/BSIZE);
    if (addr == 0)
      break;
    struct buf *bp = bread(addr);
    m = BSIZE - off%BSIZE;
    if (m > n-tot)
      m = n-tot;
    memcpy(dst, bp->data + off%BSIZE, m);
    brelse(bp);  // 添加缺失的 brelse
  }
  return tot;
}

int writei(struct inode *ip, char *src, uint32 off, uint32 n) {
  if (off > ip->size || off + n < off)
    return 0;
  if (off + n > MAXFILE*BSIZE)
    return 0;
    
  uint32 tot, m;
  for (tot=0; tot<n; tot+=m, off+=m, src+=m) {
    uint32 addr = bmap(ip, off/BSIZE);
    if (addr == 0)
      break;
    struct buf *bp = bread(addr);
    m = BSIZE - off%BSIZE;
    if (m > n-tot)
      m = n-tot;
    memcpy(bp->data + off%BSIZE, src, m);
    bwrite(bp);
    brelse(bp);  // 添加缺失的 brelse
  }
  
  if (off > ip->size)
    ip->size = off;
  iupdate(ip);
  return tot;
}

void stati(struct inode *ip, struct stat *st) {
  st->dev = ip->dev;
  st->ino = ip->inum;
  st->type = ip->type;
  st->nlink = ip->nlink;
  st->size = ip->size;
}

int namecmp(const char *s, const char *t) {
  return strncmp(s, t, DIRSIZ);
}

struct inode* dirlookup(struct inode *dp, char *name, uint32 *poff) {
  if (dp->type != T_DIR)
    return 0;
    
  struct dirent de;
  for (uint32 off = 0; off < dp->size; off += sizeof(de)) {
    if (readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
      break;
    if (de.inum == 0)
      continue;
    if (namecmp(name, de.name) == 0) {
      if (poff)
        *poff = off;
      return iget(dp->dev, de.inum);
    }
  }
  return 0;
}

int dirlink(struct inode *dp, char *name, uint32 inum) {
  struct inode *ip;
  
  if ((ip = dirlookup(dp, name, 0)) != 0) {
    iput(ip);
    return -1;
  }
  
  struct dirent de;
  uint32 off;
  for (off = 0; off < dp->size; off += sizeof(de)) {
    if (readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
      break;
    if (de.inum == 0)
      break;
  }
  
  strncpy(de.name, name, DIRSIZ);
  de.inum = inum;
  if (writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
    return -1;
  return 0;
}

static char* skipelem(char *path, char *name) {
  while (*path == '/')
    path++;
  if (*path == 0)
    return 0;
  char *s = path;
  while (*path != '/' && *path != 0)
    path++;
  int len = path - s;
  if (len >= DIRSIZ)
    len = DIRSIZ;
  memcpy(name, s, len);
  if (len < DIRSIZ)
    name[len] = 0;
  while (*path == '/')
    path++;
  return path;
}

struct inode* namei(char *path) {
  char name[DIRSIZ];
  return nameiparent(path, name);
}

struct inode* nameiparent(char *path, char *name) {
  struct inode *ip, *next;
  
  if (*path == '/')
    ip = iget(0, ROOTINO);
  else
    ip = iget(0, ROOTINO);
    
  while ((path = skipelem(path, name)) != 0) {
    ilock(ip);
    if (ip->type != T_DIR) {
      iunlockput(ip);
      return 0;
    }
    if ((next = dirlookup(ip, name, 0)) == 0) {
      iunlockput(ip);
      return 0;
    }
    iunlockput(ip);
    ip = next;
  }
  return ip;
}

// ============ 文件系统初始化 ============
void fsinit(void) {
  printf("[exp7] File system initializing...\n");
  
  // 初始化超级块
  sb.magic = FSMAGIC;
  sb.size = FSSIZE;
  sb.nblocks = FSSIZE - 100;
  sb.ninodes = NINODES;
  sb.nlog = 10;
  sb.logstart = 2;
  sb.inodestart = 2 + sb.nlog;
  sb.bmapstart = sb.inodestart + NINODES/8 + 1;
  
  printf("[exp7] FS: size=%d nblocks=%d ninodes=%d\n", 
         sb.size, sb.nblocks, sb.ninodes);
  printf("[exp7] Layout: log=%d-%d inode=%d bitmap=%d data=%d-end\n",
         sb.logstart, sb.logstart + sb.nlog - 1, 
         sb.inodestart, sb.bmapstart, sb.bmapstart + 1);
  
  // 初始化块缓存
  binit();
  
  // 初始化inode缓存
  iinit();
  
  // 初始化文件表
  fileinit();
  
  // 初始化日志系统
  log_init(0, &sb);
  
  // 创建根目录
  struct inode *root = ialloc(0, T_DIR);
  if (root) {
    ilock(root);  // 锁定以便后续操作
    root->nlink = 1;
    root->size = 0;
    iupdate(root);
    
    // 添加 . 和 ..
    begin_op();
    dirlink(root, ".", root->inum);
    dirlink(root, "..", root->inum);
    end_op();
    
    iunlockput(root);
    printf("[exp7] Root directory created (ino=%d)\n", ROOTINO);
  }
  
  printf("[exp7] File system initialized successfully\n");
}

// ============ 调试和统计函数 ============

static uint32 count_free_blocks(void) {
  uint32 free = 0;
  for (uint32 b = 0; b < sb.size; b += BSIZE*8) {
    struct buf *bp = bread(sb.bmapstart + b/(BSIZE*8));
    for (int bi = 0; bi < BSIZE*8 && b + bi < sb.size; bi++) {
      int m = 1 << (bi % 8);
      if ((bp->data[bi/8] & m) == 0) {
        free++;
      }
    }
    brelse(bp);
  }
  return free;
}

static uint32 count_free_inodes(void) {
  uint32 free = 0;
  for (uint32 inum = 1; inum < sb.ninodes; inum++) {
    struct buf *bp = bread(sb.inodestart + inum/8);
    struct dinode *dip = (struct dinode*)bp->data + inum%8;
    if (dip->type == 0) {
      free++;
    }
    brelse(bp);
  }
  return free;
}

void debug_filesystem_state(void) {
  printf("\n[exp7-debug] === Filesystem Debug Info ===\n");
  printf("[exp7-debug] Superblock:\n");
  printf("[exp7-debug]   magic: 0x%x\n", sb.magic);
  printf("[exp7-debug]   size: %d blocks\n", sb.size);
  printf("[exp7-debug]   nblocks: %d\n", sb.nblocks);
  printf("[exp7-debug]   ninodes: %d\n", sb.ninodes);
  printf("[exp7-debug]   nlog: %d\n", sb.nlog);
  printf("[exp7-debug]   logstart: %d\n", sb.logstart);
  printf("[exp7-debug]   inodestart: %d\n", sb.inodestart);
  printf("[exp7-debug]   bmapstart: %d\n", sb.bmapstart);
  
  printf("[exp7-debug] Resource usage:\n");
  printf("[exp7-debug]   Free blocks: %d/%d\n", count_free_blocks(), sb.size);
  printf("[exp7-debug]   Free inodes: %d/%d\n", count_free_inodes(), sb.ninodes);
  
  // 获取块缓存统计
  uint64 reads, writes, hits, misses;
  get_disk_stats(&reads, &writes, &hits, &misses);
  
  printf("[exp7-debug] Buffer cache:\n");
  printf("[exp7-debug]   Cache hits: %lu\n", (unsigned long)hits);
  printf("[exp7-debug]   Cache misses: %lu\n", (unsigned long)misses);
  if (hits + misses > 0) {
    uint64 total = hits + misses;
    printf("[exp7-debug]   Hit rate: %lu%%\n", 
           (unsigned long)(hits * 100 / total));
  }
}

void debug_inode_usage(void) {
  printf("\n[exp7-debug] === Inode Usage ===\n");
  int used = 0;
  for (int i = 0; i < NINODES; i++) {
    struct inode *ip = &icache.inode[i];
    if (ip->ref > 0) {
      printf("[exp7-debug] Inode %d: inum=%d ref=%d type=%d size=%d\n",
             i, ip->inum, ip->ref, ip->type, ip->size);
      used++;
    }
  }
  printf("[exp7-debug] Total inodes in cache: %d/%d\n", used, NINODES);
}

void debug_disk_io(void) {
  printf("\n[exp7-debug] === Disk I/O Statistics ===\n");
  uint64 reads, writes, hits, misses;
  get_disk_stats(&reads, &writes, &hits, &misses);
  printf("[exp7-debug] Disk reads: %lu\n", (unsigned long)reads);
  printf("[exp7-debug] Disk writes: %lu\n", (unsigned long)writes);
  printf("[exp7-debug] Total I/O operations: %lu\n", 
         (unsigned long)(reads + writes));
}
