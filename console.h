#ifndef CONSOLE_H
#define CONSOLE_H

// 基本控制台函数
void console_putc(char c);
void console_puts(const char *s);

// 清屏功能
void clear_screen(void);
void clear_line(void);

// 光标控制
void goto_xy(int x, int y);

// 颜色输出
void printf_color(int color, char *fmt, ...);

// 颜色定义
#define COLOR_BLACK   30
#define COLOR_RED     31
#define COLOR_GREEN   32
#define COLOR_YELLOW  33
#define COLOR_BLUE    34
#define COLOR_MAGENTA 35
#define COLOR_CYAN    36
#define COLOR_WHITE   37

#endif