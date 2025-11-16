// 控制台功能实现
#include "types.h"
#include "uart.h"
#include "console.h"
#include "stdarg.h"

// 前置声明
static void sprintf_simple(char *buf, const char *fmt, ...);

// 控制台输出单个字符
void console_putc(char c)
{
    uart_putc(c);
}

// 控制台输出字符串
void console_puts(const char *s)
{
    while(*s) {
        console_putc(*s);
        s++;
    }
}

// 清屏功能 
void clear_screen(void) {
  // 清当前屏幕内容
  console_puts("\033[2J");
  // 清滚动缓冲区
  console_puts("\033[3J");
  // 光标回左上角
  console_puts("\033[H");
}

// 清除当前行
void clear_line(void) {
  // EL 0 清除到行尾，然后回车刷新行首
  console_puts("\033[K\r");
}

// 光标定位
void goto_xy(int x, int y)
{
    char buf[32];
    sprintf_simple(buf, "\033[%d;%dH", y + 1, x + 1);   // 格式化输出坐标
    console_puts(buf);
}

// 简单的sprintf实现（仅支持%d）
static void sprintf_simple(char *buf, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    
    // 仅处理 "\033[%d;%dH" 格式
    if(fmt[0] == '\033' && fmt[1] == '[') {     // 如果是颜色转义序列
        int y = va_arg(ap, int);    // 获取y坐标
        int x = va_arg(ap, int);    // 获取x坐标
        
        buf[0] = '\033';
        buf[1] = '[';
        // 简单的数字转字符串
        int pos = 2;
        if(y >= 10) buf[pos++] = '0' + y/10;
        buf[pos++] = '0' + y%10;
        buf[pos++] = ';';
        if(x >= 10) buf[pos++] = '0' + x/10;
        buf[pos++] = '0' + x%10;
        buf[pos++] = 'H';
        buf[pos] = '\0';
    }
    
    va_end(ap);
}

// 颜色输出
void printf_color(int color, char *fmt, ...)
{
    char color_code[16];
    
    // 构建颜色转义序列
    color_code[0] = '\033';
    color_code[1] = '[';
    color_code[2] = '0' + (color / 10);
    color_code[3] = '0' + (color % 10);
    color_code[4] = 'm';
    color_code[5] = '\0';
    
    // 输出颜色转义序列
    console_puts(color_code);
    
    // 输出字符串
    console_puts(fmt);
    
    // 重置颜色
    console_puts("\033[0m");
}