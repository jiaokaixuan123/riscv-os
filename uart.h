// UART寄存器地址
#define UART0 0x10000000L

// UART寄存器偏移
#define UART_RHR 0                 // 接收保持寄存器
#define UART_THR 0                 // 发送保持寄存器
#define UART_IER 1                 // 中断使能寄存器
#define UART_FCR 2                 // FIFO控制寄存器
#define UART_ISR 2                 // 中断状态寄存器
#define UART_LCR 3                 // 线路控制寄存器
#define UART_LSR 5                 // 线路状态寄存器

// LSR状态位
#define UART_LSR_THRE (1 << 5)     // 发送保持寄存器空

/* 函数声明 */
void uart_init(void);
void uart_putc(char c);
void uart_puts(char *s);

