#include "string.h"
#include "types.h"

void* memset(void *dst, int c, uint n) {
    uint i;
    unsigned char *d = (unsigned char*)dst;
    for (i = 0; i < n; i++) {
        d[i] = (unsigned char)c;
    }
    return dst;
}


void* memmove(void *dst, const void *src, uint n)   //复制
{
  const char *s;
  char *d;

  s = src;
  d = dst;
  if(s < d && s + n > d){
    s += n;
    d += n;
    while(n-- > 0)
      *--d = *--s;
  } else
    while(n-- > 0)
      *d++ = *s++;

  return dst;
}

int strncmp(const char *p, const char *q, uint n)   //比较字符串
{
  while(n > 0 && *p && *p == *q)
    n--, p++, q++;
  if(n == 0)
    return 0;
  return (uchar)*p - (uchar)*q;
}

char* strncpy(char *s, const char *t, int n)   //复制字符串
{
  char *os;

  os = s;
  while(n-- > 0 && (*s++ = *t++) != 0)
    ;
  while(n-- > 0)
    *s++ = 0;
  return os;
}

void *memcpy(void *dst, const void *src, unsigned long n){
  unsigned char *d = (unsigned char*)dst;
  const unsigned char *s = (const unsigned char*)src;
  for(unsigned long i=0;i<n;i++) d[i]=s[i];
  return dst;
}