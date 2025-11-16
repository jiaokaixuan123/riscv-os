#include "types.h"
#include "uart.h"
#include <stdarg.h>

static char digits[] = "0123456789abcdef";

// 输出单个字符到控制台
static void putc(int c)
{
  uart_putc(c);
}

// 打印整数
static void printint(int xx, int base, int sign)
{
  char buf[32];
  int i;
  uint x;

  //输出符号
  if(sign && (sign = xx < 0))
    x = -xx;
  else
    x = (uint)xx;

  i = 0;
  //输出数字
  do {
    buf[i++] = digits[x % base];
  } while((x /= base) != 0);

  if(sign)
    buf[i++] = '-';

  while(--i >= 0)
    putc(buf[i]);
}

static void printlong(long v, int base){
  char buf[32]; int i=0; unsigned long x;
  int neg = (v < 0);
  x = neg ? (unsigned long)(-v) : (unsigned long)v;
  do { buf[i++] = digits[x % base]; } while((x /= base) != 0);
  if(neg) buf[i++]='-';
  while(--i >= 0) putc(buf[i]);
}

static void printulong(unsigned long v, int base){
  char buf[32]; int i=0; unsigned long x=v;
  do { buf[i++] = digits[x % base]; } while((x /= base) != 0);
  while(--i >= 0) putc(buf[i]);
}

// 打印指针
static void printptr(uint64 x)
{
  putc('0');
  putc('x');
  for (unsigned long i = 0UL; i < (sizeof(uint64) * 2UL); i++, x <<= 4)
    putc(digits[(x >> ((sizeof(uint64) * 8UL) - 4)) & 0xF]);
}

// 格式化输出主函数
void printf(char *fmt, ...)
{
  va_list ap;
  int i, c;
  char *s;

  if (fmt == 0)
    return;

  va_start(ap, fmt);
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){
      putc(c);      // 非格式化字符直接输出
      continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    int longflag = 0;
    if(c=='l'){ longflag = 1; c = fmt[++i] & 0xff; if(c==0) break; }
    switch(c){      // 格式化输出
    case 'd':
      if(longflag) printlong(va_arg(ap,long),10); else printint(va_arg(ap,int),10,1);
      break;
    case 'u':
      if(longflag) printulong(va_arg(ap,unsigned long),10); else printint(va_arg(ap,int),10,0);
      break;
    case 'x':
      if(longflag) printulong(va_arg(ap,unsigned long),16); else printint(va_arg(ap,int),16,0);
      break;
    case 'p':
      printptr(va_arg(ap,uint64));
      break;
    case 's':
      s = va_arg(ap,char*); if(!s) s = "(null)"; while(*s) putc(*s++);
      break;
    case 'c':
      putc(va_arg(ap,int));
      break;
    case '%':
      putc('%');
      break;
    default:
      // 输出原样，包含可能的'l'
      putc('%'); if(longflag) putc('l'); putc(c);
      break;
    }
  }
  va_end(ap);
}