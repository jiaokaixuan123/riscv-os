#ifndef TYPES_H
#define TYPES_H

#ifndef __ASSEMBLER__

#include "assert.h"

/* 基本类型定义*/
typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int  uint32;
typedef unsigned long uint64;

#define NULL ((void*)0)

// 可变参数支持
typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

#endif /* __ASSEMBLER__ */

#endif