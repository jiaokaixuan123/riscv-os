// 创建硬链接：多个目录项（文件名）指向同一个 inode
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if(argc != 3){
    fprintf(2, "Usage: ln old new\n");  
    exit(1);
  }
  if(link(argv[1], argv[2]) < 0)    // 创建硬链接
    fprintf(2, "link %s %s: failed\n", argv[1], argv[2]);   // 创建硬链接失败
  exit(0);
}
