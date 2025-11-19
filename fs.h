#ifndef FS_H
#define FS_H

#include "types.h"

// 文件系统参数
#define BSIZE        512  // 块大小
#define NDIRECT      12   // 直接块数量
#define NINDIRECT    (BSIZE / sizeof(uint32))
#define MAXFILE      (NDIRECT + NINDIRECT)
#define DIRSIZ       14   // 目录名最大长度
#define ROOTINO      1    // 根目录inode号
#define NINODES      200  // 最大inode数
#define NFILE        100  // 最大打开文件数
#define MAXOPBLOCKS  10   // 最大操作块数
#define LOGSIZE      (MAXOPBLOCKS*3)  // 日志大小
#define FSSIZE       1000 // 文件系统总块数

// 磁盘布局:
// [ boot block | super block | log | inode blocks | free bit map | data blocks ]

// On-disk file system format
// 超级块结构
struct superblock {
  uint32 magic;        // Must be FSMAGIC
  uint32 size;         // Size of file system image (blocks)
  uint32 nblocks;      // Number of data blocks
  uint32 ninodes;      // Number of inodes
  uint32 nlog;         // Number of log blocks
  uint32 logstart;     // Block number of first log block
  uint32 inodestart;   // Block number of first inode block
  uint32 bmapstart;    // Block number of first free map block
};

#define FSMAGIC 0x10203040

// inode结构
#define T_DIR     1   // Directory
#define T_FILE    2   // File
#define T_DEVICE  3   // Device

struct dinode {
  short type;              // File type
  short major;             // Major device number (T_DEVICE only)
  short minor;             // Minor device number (T_DEVICE only)
  short nlink;             // Number of links to inode in file system
  uint32 size;             // Size of file (bytes)
  uint32 addrs[NDIRECT+1]; // Data block addresses
};

// 内存中的inode
struct inode {
  uint32 dev;           // Device number
  uint32 inum;          // Inode number
  int ref;              // Reference count
  int valid;            // inode has been read from disk?

  short type;           // copy of disk inode
  short major;
  short minor;
  short nlink;
  uint32 size;
  uint32 addrs[NDIRECT+1];
};

// 目录项
#define DIRENT_SIZE sizeof(struct dirent)

struct dirent {
  uint16 inum;
  char name[DIRSIZ];
};

// 日志头
struct logheader {
  int n;
  int block[LOGSIZE];
};

// 块缓存结构（完整定义）
struct buf {
  int valid;
  int dirty;
  uint32 blockno;
  uint32 refcnt;
  uint8 data[BSIZE];
};

// 文件描述符
struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE, FD_DEVICE } type;
  int ref; // reference count
  char readable;
  char writable;
  struct inode *ip;
  uint32 off;
};

// stat结构 (完整定义提前，避免与原型产生不一致的警告)
struct stat {
  int dev;     // File system's disk device
  uint32 ino;  // Inode number
  short type;  // Type of file
  short nlink; // Number of links to file
  uint64 size; // Size of file in bytes
};

// 文件系统API
void            fsinit(void);
int             dirlink(struct inode*, char*, uint32);
struct inode*   dirlookup(struct inode*, char*, uint32*);
struct inode*   ialloc(uint32, short);
struct inode*   idup(struct inode*);
void            iinit(void);
void            ilock(struct inode*);
void            iput(struct inode*);
void            iunlock(struct inode*);
void            iunlockput(struct inode*);
void            iupdate(struct inode*);
int             namecmp(const char*, const char*);
struct inode*   namei(char*);
struct inode*   nameiparent(char*, char*);
int             readi(struct inode*, char*, uint32, uint32);
void            stati(struct inode*, struct stat*);
int             writei(struct inode*, char*, uint32, uint32);
void            itrunc(struct inode*);

// 文件API
struct file*    filealloc(void);
void            fileclose(struct file*);
struct file*    filedup(struct file*);
int             fileread(struct file*, char*, int n);
int             filestat(struct file*, struct stat*);
int             filewrite(struct file*, char*, int n);

// 日志API
void            log_init(int dev, struct superblock *sb);
void            begin_op(void);
void            end_op(void);
void            log_write_block(struct buf *b);

// 调试和统计API
void            debug_filesystem_state(void);
void            debug_inode_usage(void);
void            debug_disk_io(void);

#endif // FS_H
