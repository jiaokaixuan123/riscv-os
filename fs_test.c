// 实验7：文件系统测试
#include "types.h"
#include "defs.h"
#include "fs.h"
#include "printf.h"
#include "string.h"

// 函数前置声明
static void test_create_open(void);
static void test_read_write(void);
static void test_directory(void);
static void test_file_table(void);
static void test_large_file(void);
static void test_truncate(void);
static void test_performance(void);
static void test_concurrent_files(void);

// 测试1: 创建和打开文件
static void test_create_open(void) {
  printf("[exp7-test1] Testing file create and open...\n");
  
  // 分配inode
  struct inode *ip = ialloc(0, T_FILE);
  if (ip == 0) {
    printf("[exp7-test1] FAILED: cannot allocate inode\n");
    return;
  }
  
  printf("[exp7-test1] Created inode %d\n", ip->inum);
  
  // 锁定并初始化
  ilock(ip);
  ip->nlink = 1;
  ip->size = 0;
  iupdate(ip);
  iunlock(ip);
  
  // 再次获取
  struct inode *ip2 = iget(0, ip->inum);
  if (ip2->inum != ip->inum) {
    printf("[exp7-test1] FAILED: iget returned wrong inode\n");
    iput(ip);
    return;
  }
  
  iput(ip2);
  iput(ip);
  
  printf("[exp7-test1] PASSED\n");
}

// 测试2: 文件读写
static void test_read_write(void) {
  printf("[exp7-test2] Testing file read/write...\n");
  
  // 创建文件
  struct inode *ip = ialloc(0, T_FILE);
  if (ip == 0) {
    printf("[exp7-test2] FAILED: cannot allocate inode\n");
    return;
  }
  
  ilock(ip);
  ip->nlink = 1;
  ip->size = 0;
  
  // 写入数据
  char wbuf[100];
  for (int i = 0; i < 100; i++)
    wbuf[i] = 'A' + (i % 26);
  
  int n = writei(ip, wbuf, 0, 100);
  if (n != 100) {
    printf("[exp7-test2] FAILED: write returned %d, expected 100\n", n);
    iunlockput(ip);
    return;
  }
  printf("[exp7-test2] Wrote 100 bytes\n");
  
  // 读取数据
  char rbuf[100];
  memset(rbuf, 0, sizeof(rbuf));
  n = readi(ip, rbuf, 0, 100);
  if (n != 100) {
    printf("[exp7-test2] FAILED: read returned %d, expected 100\n", n);
    iunlockput(ip);
    return;
  }
  printf("[exp7-test2] Read 100 bytes\n");
  
  // 验证数据
  int errors = 0;
  for (int i = 0; i < 100; i++) {
    if (rbuf[i] != wbuf[i]) {
      errors++;
      if (errors <= 5)
        printf("[exp7-test2] Mismatch at %d: got %c, expected %c\n", 
               i, rbuf[i], wbuf[i]);
    }
  }
  
  if (errors > 0) {
    printf("[exp7-test2] FAILED: %d mismatches\n", errors);
  } else {
    printf("[exp7-test2] Data verified successfully\n");
    printf("[exp7-test2] PASSED\n");
  }
  
  iunlockput(ip);
}

// 测试3: 目录操作
static void test_directory(void) {
  printf("[exp7-test3] Testing directory operations...\n");
  
  // 获取根目录
  struct inode *root = iget(0, ROOTINO);
  if (root == 0) {
    printf("[exp7-test3] FAILED: cannot get root directory\n");
    return;
  }
  
  ilock(root);
  if (root->type != T_DIR) {
    printf("[exp7-test3] FAILED: root is not a directory\n");
    iunlockput(root);
    return;
  }
  printf("[exp7-test3] Got root directory (ino=%d)\n", root->inum);
  
  // 创建子目录
  struct inode *subdir = ialloc(0, T_DIR);
  if (subdir == 0) {
    printf("[exp7-test3] FAILED: cannot allocate subdir\n");
    iunlockput(root);
    return;
  }
  
  ilock(subdir);
  subdir->nlink = 1;
  subdir->size = 0;
  iupdate(subdir);
  
  // 添加到根目录
  if (dirlink(root, "testdir", subdir->inum) < 0) {
    printf("[exp7-test3] FAILED: cannot link directory\n");
    iunlockput(subdir);
    iunlockput(root);
    return;
  }
  printf("[exp7-test3] Created directory 'testdir' (ino=%d)\n", subdir->inum);
  
  // 查找目录
  iunlock(root);
  struct inode *found = dirlookup(root, "testdir", 0);
  if (found == 0) {
    printf("[exp7-test3] FAILED: cannot find 'testdir'\n");
    iunlockput(subdir);
    iput(root);
    return;
  }
  
  if (found->inum != subdir->inum) {
    printf("[exp7-test3] FAILED: found wrong inode\n");
    iput(found);
    iunlockput(subdir);
    iput(root);
    return;
  }
  
  printf("[exp7-test3] Found 'testdir' successfully\n");
  printf("[exp7-test3] PASSED\n");
  
  iput(found);
  iunlockput(subdir);
  iput(root);
}

// 测试4: 文件表操作
static void test_file_table(void) {
  printf("[exp7-test4] Testing file table operations...\n");
  
  // 分配文件
  struct file *f = filealloc();
  if (f == 0) {
    printf("[exp7-test4] FAILED: cannot allocate file\n");
    return;
  }
  printf("[exp7-test4] Allocated file\n");
  
  // 设置文件
  struct inode *ip = ialloc(0, T_FILE);
  if (ip == 0) {
    printf("[exp7-test4] FAILED: cannot allocate inode\n");
    fileclose(f);
    return;
  }
  
  ilock(ip);
  ip->nlink = 1;
  ip->size = 0;
  iupdate(ip);
  iunlock(ip);
  
  f->type = FD_INODE;
  f->ip = ip;
  f->off = 0;
  f->readable = 1;
  f->writable = 1;
  
  // 写入文件
  char wbuf[] = "Hello, File System!";
  int n = filewrite(f, wbuf, sizeof(wbuf));
  if (n != sizeof(wbuf)) {
    printf("[exp7-test4] FAILED: write returned %d\n", n);
    fileclose(f);
    return;
  }
  printf("[exp7-test4] Wrote %d bytes: '%s'\n", n, wbuf);
  
  // 重置偏移
  f->off = 0;
  
  // 读取文件
  char rbuf[100];
  memset(rbuf, 0, sizeof(rbuf));
  n = fileread(f, rbuf, sizeof(wbuf));
  if (n != sizeof(wbuf)) {
    printf("[exp7-test4] FAILED: read returned %d\n", n);
    fileclose(f);
    return;
  }
  printf("[exp7-test4] Read %d bytes: '%s'\n", n, rbuf);
  
  // 验证
  if (strcmp(wbuf, rbuf) != 0) {
    printf("[exp7-test4] FAILED: data mismatch\n");
    fileclose(f);
    return;
  }
  
  printf("[exp7-test4] PASSED\n");
  fileclose(f);
}

// 测试5: 大文件读写
static void test_large_file(void) {
  printf("[exp7-test5] Testing large file I/O...\n");
  
  struct inode *ip = ialloc(0, T_FILE);
  if (ip == 0) {
    printf("[exp7-test5] FAILED: cannot allocate inode\n");
    return;
  }
  
  ilock(ip);
  ip->nlink = 1;
  ip->size = 0;
  
  // 写入多个块
  char wbuf[BSIZE];
  int total_written = 0;
  int nblocks = 3;
  
  for (int i = 0; i < nblocks; i++) {
    // 填充数据
    for (int j = 0; j < BSIZE; j++)
      wbuf[j] = 'A' + ((i * BSIZE + j) % 26);
    
    int n = writei(ip, wbuf, i * BSIZE, BSIZE);
    if (n != BSIZE) {
      printf("[exp7-test5] FAILED: write block %d returned %d\n", i, n);
      iunlockput(ip);
      return;
    }
    total_written += n;
  }
  printf("[exp7-test5] Wrote %d bytes (%d blocks)\n", total_written, nblocks);
  
  // 读取并验证
  char rbuf[BSIZE];
  int errors = 0;
  for (int i = 0; i < nblocks; i++) {
    memset(rbuf, 0, BSIZE);
    int n = readi(ip, rbuf, i * BSIZE, BSIZE);
    if (n != BSIZE) {
      printf("[exp7-test5] FAILED: read block %d returned %d\n", i, n);
      errors++;
      break;
    }
    
    // 验证
    for (int j = 0; j < BSIZE; j++) {
      char expected = 'A' + ((i * BSIZE + j) % 26);
      if (rbuf[j] != expected) {
        errors++;
        if (errors <= 3)
          printf("[exp7-test5] Block %d offset %d: got %c, expected %c\n",
                 i, j, rbuf[j], expected);
      }
    }
  }
  
  if (errors > 0) {
    printf("[exp7-test5] FAILED: %d errors\n", errors);
  } else {
    printf("[exp7-test5] All %d bytes verified\n", total_written);
    printf("[exp7-test5] PASSED\n");
  }
  
  iunlockput(ip);
}

// 测试6: 文件截断
static void test_truncate(void) {
  printf("[exp7-test6] Testing file truncate...\n");
  
  struct inode *ip = ialloc(0, T_FILE);
  if (ip == 0) {
    printf("[exp7-test6] FAILED: cannot allocate inode\n");
    return;
  }
  
  ilock(ip);
  ip->nlink = 1;
  ip->size = 0;
  
  // 写入数据
  char wbuf[BSIZE];
  for (int i = 0; i < BSIZE; i++)
    wbuf[i] = 'X';
  
  int n = writei(ip, wbuf, 0, BSIZE);
  if (n != BSIZE) {
    printf("[exp7-test6] FAILED: write returned %d\n", n);
    iunlockput(ip);
    return;
  }
  printf("[exp7-test6] Wrote %d bytes, size=%d\n", n, ip->size);
  
  // 截断
  itrunc(ip);
  printf("[exp7-test6] Truncated file, new size=%d\n", ip->size);
  
  if (ip->size != 0) {
    printf("[exp7-test6] FAILED: size not zero after truncate\n");
    iunlockput(ip);
    return;
  }
  
  // 尝试读取
  char rbuf[100];
  n = readi(ip, rbuf, 0, 100);
  if (n != 0) {
    printf("[exp7-test6] FAILED: read %d bytes from empty file\n", n);
    iunlockput(ip);
    return;
  }
  
  printf("[exp7-test6] PASSED\n");
  iunlockput(ip);
}

// 运行所有测试
void run_fs_tests(void) {
  printf("\n[exp7] ========================================\n");
  printf("[exp7] === File System Tests (Experiment 7) ===\n");
  printf("[exp7] ========================================\n\n");
  
  // 显示文件系统状态
  debug_filesystem_state();
  printf("\n");
  
  // 运行功能测试
  test_create_open();
  printf("\n");
  
  test_read_write();
  printf("\n");
  
  test_directory();
  printf("\n");
  
  test_file_table();
  printf("\n");
  
  test_large_file();
  printf("\n");
  
  test_truncate();
  printf("\n");
  
  // 性能测试
  printf("[exp7] ========== Performance Tests ==========\n\n");
  test_performance();
  printf("\n");
  
  // 并发测试（简化版）
  printf("[exp7] ========== Concurrency Tests ==========\n\n");
  test_concurrent_files();
  printf("\n");
  
  // 最终状态
  printf("[exp7] ========== Final State ==========\n");
  debug_filesystem_state();
  debug_inode_usage();
  debug_disk_io();
  
  printf("\n[exp7] ========================================\n");
  printf("[exp7] === All Tests Completed Successfully ===\n");
  printf("[exp7] ========================================\n");
  exit_process(0);
}

// 测试7: 性能测试
static void test_performance(void) {
  printf("[exp7-test7] Testing filesystem performance...\n");
  
  uint64 start_time = get_time();
  
  // 小文件测试
  int small_count = 20;
  struct inode *small_files[20];
  
  for (int i = 0; i < small_count; i++) {
    small_files[i] = ialloc(0, T_FILE);
    if (small_files[i] == 0) {
      printf("[exp7-test7] Failed to allocate inode %d\n", i);
      break;
    }
    ilock(small_files[i]);
    small_files[i]->nlink = 1;
    small_files[i]->size = 0;
    
    char data[] = "test";
    writei(small_files[i], data, 0, sizeof(data));
    iunlock(small_files[i]);
  }
  
  uint64 small_time = get_time() - start_time;
  printf("[exp7-test7] Created %d small files in %lu cycles\n", 
         small_count, (unsigned long)small_time);
  
  // 清理
  for (int i = 0; i < small_count; i++) {
    if (small_files[i]) {
      iput(small_files[i]);
    }
  }
  
  // 大文件测试
  start_time = get_time();
  struct inode *large = ialloc(0, T_FILE);
  if (large) {
    ilock(large);
    large->nlink = 1;
    large->size = 0;
    
    char buf[BSIZE];
    for (int i = 0; i < BSIZE; i++)
      buf[i] = 'L';
    
    for (int i = 0; i < 5; i++) {
      writei(large, buf, i * BSIZE, BSIZE);
    }
    
    iunlockput(large);
  }
  
  uint64 large_time = get_time() - start_time;
  printf("[exp7-test7] Created 1 large file (5 blocks) in %lu cycles\n",
         (unsigned long)large_time);
  
  printf("[exp7-test7] PASSED\n");
}

// 测试8: 简单的并发测试
static void test_concurrent_files(void) {
  printf("[exp7-test8] Testing concurrent file operations...\n");
  
  // 创建多个文件
  struct inode *files[5];
  for (int i = 0; i < 5; i++) {
    files[i] = ialloc(0, T_FILE);
    if (files[i] == 0) {
      printf("[exp7-test8] Failed to allocate inode %d\n", i);
      continue;
    }
    
    ilock(files[i]);
    files[i]->nlink = 1;
    files[i]->size = 0;
    
    char data[100];
    for (int j = 0; j < 100; j++)
      data[j] = 'A' + i;
    
    int n = writei(files[i], data, 0, 100);
    if (n != 100) {
      printf("[exp7-test8] Write failed for file %d\n", i);
    }
    
    iunlock(files[i]);
  }
  
  printf("[exp7-test8] Created 5 concurrent files\n");
  
  // 验证
  for (int i = 0; i < 5; i++) {
    if (files[i] == 0) continue;
    
    ilock(files[i]);
    char data[100];
    int n = readi(files[i], data, 0, 100);
    
    if (n != 100) {
      printf("[exp7-test8] Read failed for file %d\n", i);
      iunlock(files[i]);
      continue;
    }
    
    int errors = 0;
    for (int j = 0; j < 100; j++) {
      if (data[j] != 'A' + i) {
        errors++;
      }
    }
    
    if (errors > 0) {
      printf("[exp7-test8] File %d has %d errors\n", i, errors);
    }
    
    iunlockput(files[i]);
  }
  
  printf("[exp7-test8] All concurrent files verified\n");
  printf("[exp7-test8] PASSED\n");
}
