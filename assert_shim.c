// assert_shim.c — 提供裸机下 assert() 需要的钩子
#include "types.h"
#include "printf.h"

// 与 newlib/gcc 的 assert.h 约定保持一致
// 形参含义：文件名、行号、函数名、表达式字符串
void __assert_func(const char *file, int line, const char *func, const char *expr) {
  printf("assert failed: %s:%d: %s: %s\n",
         file ? file : "(unknown)",
         line,
         func ? func : "(func)",
         expr ? expr : "(expr)");
  // 这里选择直接停机（也可以触发断点/进入死循环）
  for (;;) { }
}

// 有些工具链用的是 __assert_fail；做个别名以防万一
void __assert_fail(const char *expr, const char *file, int line, const char *func) {
  __assert_func(file, line, func, expr);
  for (;;) { }
}
