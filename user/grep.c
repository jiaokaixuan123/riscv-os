// grep.c: 简单的 grep 实现，支持基本的正则表达式匹配
// grep: 搜索文件中的模式并输出匹配的行

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

char buf[1024];
int match(char*, char*);

void
grep(char *pattern, int fd)
{
  int n, m;
  char *p, *q;

  m = 0;
  while((n = read(fd, buf+m, sizeof(buf)-m-1)) > 0){  // 读取文件内容
    m += n;                                           // 更新缓冲区中的字节数
    buf[m] = '\0';                                    // 添加字符串结束符
    p = buf;                                          // 指向缓冲区的开始
    while((q = strchr(p, '\n')) != 0){                // 查找行结束符
      *q = 0;                                    // 临时替换为字符串结束符
      if(match(pattern, p)){                    // 如果匹配模式
        *q = '\n';                         // 恢复行结束符
        write(1, p, q+1 - p);          // 输出匹配的行
      }
      p = q+1;
    }
    if(m > 0){                                 // 处理剩余部分
      m -= p - buf;                 // 计算剩余字节数
      memmove(buf, p, m);             // 将剩余部分移动到缓冲区开头
    }
  }
}

int
main(int argc, char *argv[])
{
  int fd, i;
  char *pattern;

  if(argc <= 1){
    fprintf(2, "usage: grep pattern [file ...]\n");
    exit(1);
  }
  pattern = argv[1];

  if(argc <= 2){
    grep(pattern, 0);
    exit(0);
  }

  for(i = 2; i < argc; i++){
    if((fd = open(argv[i], O_RDONLY)) < 0){
      printf("grep: cannot open %s\n", argv[i]);
      exit(1);
    }
    grep(pattern, fd);
    close(fd);
  }
  exit(0);
}

// match: 查找正则表达式 re 是否匹配文本 text

int matchhere(char*, char*);
int matchstar(int, char*, char*);

int
match(char *re, char *text)
{
  if(re[0] == '^')
    return matchhere(re+1, text);
  do{  // 不带 ^ 的模式可以匹配任何位置
    if(matchhere(re, text))
      return 1;
  }while(*text++ != '\0');
  return 0;
}

// matchhere: 查找 re 是否匹配 text 的开头
int matchhere(char *re, char *text)
{
  if(re[0] == '\0')
    return 1;
  if(re[1] == '*')
    return matchstar(re[0], re+2, text);
  if(re[0] == '$' && re[1] == '\0')
    return *text == '\0';
  if(*text!='\0' && (re[0]=='.' || re[0]==*text))
    return matchhere(re+1, text+1);
  return 0;
}

// matchstar: 查找 re 中的 c* 是否匹配 text 的开头
int matchstar(int c, char *re, char *text)
{
  do{  // a * matches zero or more instances
    if(matchhere(re, text))
      return 1;
  }while(*text!='\0' && (*text++==c || c=='.'));
  return 0;
}

