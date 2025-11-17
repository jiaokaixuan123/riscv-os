#include "uart.h"

/* 简化的UART初始化  */
void uart_init(void)
{

}

/* 输出单个字符 */
void uart_putc(char c)
{
    volatile char *uart = (volatile char *)UART0;
    
    /* 等待发送寄存器空闲，添加超时避免永久阻塞 */
    volatile char *lsr = (volatile char *)(UART0 + UART_LSR);
    int timeout = 100000;  // 超时计数器
    while ((*lsr & UART_LSR_THRE) == 0 && timeout > 0) {
        timeout--;
    }
    
    /* 如果超时就放弃发送，避免死锁 */
    if (timeout == 0) {
        return;
    }
    
    /* 发送字符 */
    *uart = c;
}

/* 输出字符串 */
void uart_puts(char *s)
{
    while (*s) {
        uart_putc(*s);
        s++;
    }
}
