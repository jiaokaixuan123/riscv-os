
kernel:     file format elf64-littleriscv


Disassembly of section .text:

0000000080000000 <_entry>:
.section .text
.global _entry
_entry:
        # 调试检查点1：输出启动标记 'S'
        li t0, 0x10000000       # UART0 基地址
    80000000:	100002b7          	lui	t0,0x10000
        li t1, 'S'              # 启动标记
    80000004:	05300313          	li	t1,83
        sb t1, 0(t0)            # 写入THR寄存器
    80000008:	00628023          	sb	t1,0(t0) # 10000000 <_entry-0x70000000>

        # 设置栈指针 
        la sp, stack_top
    8000000c:	00002117          	auipc	sp,0x2
    80000010:	29410113          	add	sp,sp,660 # 800022a0 <bss_start>

        # 调试检查点2：栈设置完成标记 'P'
        li t1, 'P'
    80000014:	05000313          	li	t1,80
        sb t1, 0(t0)
    80000018:	00628023          	sb	t1,0(t0)

000000008000001c <clear_bss_loop>:

        # 清零BSS段 
        #la t0, bss_start
        #la t1, bss_end
clear_bss_loop:
        beq t0, t1, bss_cleared
    8000001c:	00628663          	beq	t0,t1,80000028 <bss_cleared>
        sw zero, 0(t0)
    80000020:	0002a023          	sw	zero,0(t0)
        addi t0, t0, 4
    80000024:	0291                	add	t0,t0,4
        j clear_bss_loop
    80000026:	bfdd                	j	8000001c <clear_bss_loop>

0000000080000028 <bss_cleared>:

bss_cleared:
        # 调试检查点3：BSS清零完成标记 'B'
        li t0, 0x10000000
    80000028:	100002b7          	lui	t0,0x10000
        li t1, 'B'
    8000002c:	04200313          	li	t1,66
        sb t1, 0(t0)
    80000030:	00628023          	sb	t1,0(t0) # 10000000 <_entry-0x70000000>

        # 跳转到C主函数 
        call main
    80000034:	00000097          	auipc	ra,0x0
    80000038:	202080e7          	jalr	514(ra) # 80000236 <main>

000000008000003c <spin>:

        # 如果main返回，进入无限循环
spin:
        li t0, 0x10000000
    8000003c:	100002b7          	lui	t0,0x10000
        li t1, 'E'              # 错误标记：main不应该返回
    80000040:	04500313          	li	t1,69
        sb t1, 0(t0)
    80000044:	00628023          	sb	t1,0(t0) # 10000000 <_entry-0x70000000>
        j spin
    80000048:	bfd5                	j	8000003c <spin>

000000008000004a <test_printf_basic>:
#include "printf.h"
#include "console.h"

// 基础功能测试
void test_printf_basic()
{
    8000004a:	1141                	add	sp,sp,-16
    8000004c:	e406                	sd	ra,8(sp)
    8000004e:	e022                	sd	s0,0(sp)
    80000050:	0800                	add	s0,sp,16
    printf("=== Testing basic printf ===\n");
    80000052:	00001517          	auipc	a0,0x1
    80000056:	fae50513          	add	a0,a0,-82 # 80001000 <printf_color+0x846>
    8000005a:	00000097          	auipc	ra,0x0
    8000005e:	406080e7          	jalr	1030(ra) # 80000460 <printf>
    printf("Testing integer: %d\n", 42);
    80000062:	02a00593          	li	a1,42
    80000066:	00001517          	auipc	a0,0x1
    8000006a:	fba50513          	add	a0,a0,-70 # 80001020 <printf_color+0x866>
    8000006e:	00000097          	auipc	ra,0x0
    80000072:	3f2080e7          	jalr	1010(ra) # 80000460 <printf>
    printf("Testing negative: %d\n", -123);
    80000076:	f8500593          	li	a1,-123
    8000007a:	00001517          	auipc	a0,0x1
    8000007e:	fbe50513          	add	a0,a0,-66 # 80001038 <printf_color+0x87e>
    80000082:	00000097          	auipc	ra,0x0
    80000086:	3de080e7          	jalr	990(ra) # 80000460 <printf>
    printf("Testing zero: %d\n", 0);
    8000008a:	4581                	li	a1,0
    8000008c:	00001517          	auipc	a0,0x1
    80000090:	fc450513          	add	a0,a0,-60 # 80001050 <printf_color+0x896>
    80000094:	00000097          	auipc	ra,0x0
    80000098:	3cc080e7          	jalr	972(ra) # 80000460 <printf>
    printf("Testing hex: 0x%x\n", 0xABC);
    8000009c:	6585                	lui	a1,0x1
    8000009e:	abc58593          	add	a1,a1,-1348 # abc <_entry-0x7ffff544>
    800000a2:	00001517          	auipc	a0,0x1
    800000a6:	fc650513          	add	a0,a0,-58 # 80001068 <printf_color+0x8ae>
    800000aa:	00000097          	auipc	ra,0x0
    800000ae:	3b6080e7          	jalr	950(ra) # 80000460 <printf>
    printf("Testing string: %s\n", "Hello World");
    800000b2:	00001597          	auipc	a1,0x1
    800000b6:	fce58593          	add	a1,a1,-50 # 80001080 <printf_color+0x8c6>
    800000ba:	00001517          	auipc	a0,0x1
    800000be:	fd650513          	add	a0,a0,-42 # 80001090 <printf_color+0x8d6>
    800000c2:	00000097          	auipc	ra,0x0
    800000c6:	39e080e7          	jalr	926(ra) # 80000460 <printf>
    printf("Testing char: %c\n", 'X');
    800000ca:	05800593          	li	a1,88
    800000ce:	00001517          	auipc	a0,0x1
    800000d2:	fda50513          	add	a0,a0,-38 # 800010a8 <printf_color+0x8ee>
    800000d6:	00000097          	auipc	ra,0x0
    800000da:	38a080e7          	jalr	906(ra) # 80000460 <printf>
    printf("Testing percent: %%\n");
    800000de:	00001517          	auipc	a0,0x1
    800000e2:	fe250513          	add	a0,a0,-30 # 800010c0 <printf_color+0x906>
    800000e6:	00000097          	auipc	ra,0x0
    800000ea:	37a080e7          	jalr	890(ra) # 80000460 <printf>
}
    800000ee:	60a2                	ld	ra,8(sp)
    800000f0:	6402                	ld	s0,0(sp)
    800000f2:	0141                	add	sp,sp,16
    800000f4:	8082                	ret

00000000800000f6 <test_printf_edge_cases>:

// 边界条件测试
void test_printf_edge_cases()
{
    800000f6:	1141                	add	sp,sp,-16
    800000f8:	e406                	sd	ra,8(sp)
    800000fa:	e022                	sd	s0,0(sp)
    800000fc:	0800                	add	s0,sp,16
    printf("\n=== Testing edge cases ===\n");
    800000fe:	00001517          	auipc	a0,0x1
    80000102:	fda50513          	add	a0,a0,-38 # 800010d8 <printf_color+0x91e>
    80000106:	00000097          	auipc	ra,0x0
    8000010a:	35a080e7          	jalr	858(ra) # 80000460 <printf>
    printf("INT_MAX: %d\n", 2147483647);
    8000010e:	800005b7          	lui	a1,0x80000
    80000112:	fff5c593          	not	a1,a1
    80000116:	00001517          	auipc	a0,0x1
    8000011a:	fe250513          	add	a0,a0,-30 # 800010f8 <printf_color+0x93e>
    8000011e:	00000097          	auipc	ra,0x0
    80000122:	342080e7          	jalr	834(ra) # 80000460 <printf>
    printf("INT_MIN: %d\n", -2147483648);
    80000126:	800005b7          	lui	a1,0x80000
    8000012a:	00001517          	auipc	a0,0x1
    8000012e:	fde50513          	add	a0,a0,-34 # 80001108 <printf_color+0x94e>
    80000132:	00000097          	auipc	ra,0x0
    80000136:	32e080e7          	jalr	814(ra) # 80000460 <printf>
    printf("NULL string: %s\n", (char*)0);
    8000013a:	4581                	li	a1,0
    8000013c:	00001517          	auipc	a0,0x1
    80000140:	fdc50513          	add	a0,a0,-36 # 80001118 <printf_color+0x95e>
    80000144:	00000097          	auipc	ra,0x0
    80000148:	31c080e7          	jalr	796(ra) # 80000460 <printf>
    printf("Empty string: %s\n", "");
    8000014c:	00001597          	auipc	a1,0x1
    80000150:	fdc58593          	add	a1,a1,-36 # 80001128 <printf_color+0x96e>
    80000154:	00001517          	auipc	a0,0x1
    80000158:	fdc50513          	add	a0,a0,-36 # 80001130 <printf_color+0x976>
    8000015c:	00000097          	auipc	ra,0x0
    80000160:	304080e7          	jalr	772(ra) # 80000460 <printf>
    printf("Large hex: 0x%x\n", 0xFFFFFFFF);
    80000164:	55fd                	li	a1,-1
    80000166:	00001517          	auipc	a0,0x1
    8000016a:	fe250513          	add	a0,a0,-30 # 80001148 <printf_color+0x98e>
    8000016e:	00000097          	auipc	ra,0x0
    80000172:	2f2080e7          	jalr	754(ra) # 80000460 <printf>
}
    80000176:	60a2                	ld	ra,8(sp)
    80000178:	6402                	ld	s0,0(sp)
    8000017a:	0141                	add	sp,sp,16
    8000017c:	8082                	ret

000000008000017e <test_clear_screen>:

// 清屏功能测试
void test_clear_screen()
{
    8000017e:	1101                	add	sp,sp,-32
    80000180:	ec06                	sd	ra,24(sp)
    80000182:	e822                	sd	s0,16(sp)
    80000184:	1000                	add	s0,sp,32
    printf("Screen will be cleared in 3 seconds...\n");
    80000186:	00001517          	auipc	a0,0x1
    8000018a:	fda50513          	add	a0,a0,-38 # 80001160 <printf_color+0x9a6>
    8000018e:	00000097          	auipc	ra,0x0
    80000192:	2d2080e7          	jalr	722(ra) # 80000460 <printf>
    
    // 简单延时
    for(volatile int i = 0; i < 10000000; i++);
    80000196:	fe042623          	sw	zero,-20(s0)
    8000019a:	fec42703          	lw	a4,-20(s0)
    8000019e:	2701                	sext.w	a4,a4
    800001a0:	009897b7          	lui	a5,0x989
    800001a4:	67f78793          	add	a5,a5,1663 # 98967f <_entry-0x7f676981>
    800001a8:	00e7cd63          	blt	a5,a4,800001c2 <test_clear_screen+0x44>
    800001ac:	873e                	mv	a4,a5
    800001ae:	fec42783          	lw	a5,-20(s0)
    800001b2:	2785                	addw	a5,a5,1
    800001b4:	fef42623          	sw	a5,-20(s0)
    800001b8:	fec42783          	lw	a5,-20(s0)
    800001bc:	2781                	sext.w	a5,a5
    800001be:	fef758e3          	bge	a4,a5,800001ae <test_clear_screen+0x30>
    
    clear_screen();
    800001c2:	00000097          	auipc	ra,0x0
    800001c6:	570080e7          	jalr	1392(ra) # 80000732 <clear_screen>
    printf("Screen cleared! \n");
    800001ca:	00001517          	auipc	a0,0x1
    800001ce:	fbe50513          	add	a0,a0,-66 # 80001188 <printf_color+0x9ce>
    800001d2:	00000097          	auipc	ra,0x0
    800001d6:	28e080e7          	jalr	654(ra) # 80000460 <printf>
    printf("Testing goto_xy function:\n");
    800001da:	00001517          	auipc	a0,0x1
    800001de:	fc650513          	add	a0,a0,-58 # 800011a0 <printf_color+0x9e6>
    800001e2:	00000097          	auipc	ra,0x0
    800001e6:	27e080e7          	jalr	638(ra) # 80000460 <printf>
    
    goto_xy(10, 5);
    800001ea:	4595                	li	a1,5
    800001ec:	4529                	li	a0,10
    800001ee:	00000097          	auipc	ra,0x0
    800001f2:	594080e7          	jalr	1428(ra) # 80000782 <goto_xy>
    printf("position (10,5)");
    800001f6:	00001517          	auipc	a0,0x1
    800001fa:	fca50513          	add	a0,a0,-54 # 800011c0 <printf_color+0xa06>
    800001fe:	00000097          	auipc	ra,0x0
    80000202:	262080e7          	jalr	610(ra) # 80000460 <printf>
    
    goto_xy(0, 7);
    80000206:	459d                	li	a1,7
    80000208:	4501                	li	a0,0
    8000020a:	00000097          	auipc	ra,0x0
    8000020e:	578080e7          	jalr	1400(ra) # 80000782 <goto_xy>
    printf("left margin, line 7");
    80000212:	00001517          	auipc	a0,0x1
    80000216:	fbe50513          	add	a0,a0,-66 # 800011d0 <printf_color+0xa16>
    8000021a:	00000097          	auipc	ra,0x0
    8000021e:	246080e7          	jalr	582(ra) # 80000460 <printf>
    
    goto_xy(0, 10);
    80000222:	45a9                	li	a1,10
    80000224:	4501                	li	a0,0
    80000226:	00000097          	auipc	ra,0x0
    8000022a:	55c080e7          	jalr	1372(ra) # 80000782 <goto_xy>
}
    8000022e:	60e2                	ld	ra,24(sp)
    80000230:	6442                	ld	s0,16(sp)
    80000232:	6105                	add	sp,sp,32
    80000234:	8082                	ret

0000000080000236 <main>:

int i;
void main(void)
{
    80000236:	1141                	add	sp,sp,-16
    80000238:	e406                	sd	ra,8(sp)
    8000023a:	e022                	sd	s0,0(sp)
    8000023c:	0800                	add	s0,sp,16

    /* 输出启动成功标记 */
    uart_putc('M');  /* Main函数开始标记 */
    8000023e:	04d00513          	li	a0,77
    80000242:	00000097          	auipc	ra,0x0
    80000246:	02e080e7          	jalr	46(ra) # 80000270 <uart_putc>
    
    // /* 输出目标字符串 */
    // uart_puts("Hello OS\n");

    // printf功能
    test_printf_basic();
    8000024a:	00000097          	auipc	ra,0x0
    8000024e:	e00080e7          	jalr	-512(ra) # 8000004a <test_printf_basic>

    // 边界条件
    test_printf_edge_cases();
    80000252:	00000097          	auipc	ra,0x0
    80000256:	ea4080e7          	jalr	-348(ra) # 800000f6 <test_printf_edge_cases>
    
    // 清屏功能
    test_clear_screen();
    8000025a:	00000097          	auipc	ra,0x0
    8000025e:	f24080e7          	jalr	-220(ra) # 8000017e <test_clear_screen>

    /* 进入无限循环 (系统空闲状态) */
    while (1) {}
    80000262:	a001                	j	80000262 <main+0x2c>

0000000080000264 <uart_init>:
#include "uart.h"

/* 简化的UART初始化  */
void uart_init(void)
{
    80000264:	1141                	add	sp,sp,-16
    80000266:	e422                	sd	s0,8(sp)
    80000268:	0800                	add	s0,sp,16

}
    8000026a:	6422                	ld	s0,8(sp)
    8000026c:	0141                	add	sp,sp,16
    8000026e:	8082                	ret

0000000080000270 <uart_putc>:

/* 输出单个字符 */
void uart_putc(char c)
{
    80000270:	1141                	add	sp,sp,-16
    80000272:	e422                	sd	s0,8(sp)
    80000274:	0800                	add	s0,sp,16
    volatile char *uart = (volatile char *)UART0;
    
    /* 等待发送寄存器空闲 */
    volatile char *lsr = (volatile char *)(UART0 + UART_LSR);
    while ((*lsr & UART_LSR_THRE) == 0) {
    80000276:	10000737          	lui	a4,0x10000
    8000027a:	00574783          	lbu	a5,5(a4) # 10000005 <_entry-0x6ffffffb>
    8000027e:	0207f793          	and	a5,a5,32
    80000282:	dfe5                	beqz	a5,8000027a <uart_putc+0xa>
        /* 等待 */
    }
    
    /* 发送字符 */
    *uart = c;
    80000284:	100007b7          	lui	a5,0x10000
    80000288:	00a78023          	sb	a0,0(a5) # 10000000 <_entry-0x70000000>
}
    8000028c:	6422                	ld	s0,8(sp)
    8000028e:	0141                	add	sp,sp,16
    80000290:	8082                	ret

0000000080000292 <uart_puts>:

/* 输出字符串 */
void uart_puts(char *s)
{
    80000292:	1101                	add	sp,sp,-32
    80000294:	ec06                	sd	ra,24(sp)
    80000296:	e822                	sd	s0,16(sp)
    80000298:	e426                	sd	s1,8(sp)
    8000029a:	1000                	add	s0,sp,32
    8000029c:	84aa                	mv	s1,a0
    while (*s) {
    8000029e:	00054503          	lbu	a0,0(a0)
    800002a2:	c909                	beqz	a0,800002b4 <uart_puts+0x22>
        uart_putc(*s);
    800002a4:	00000097          	auipc	ra,0x0
    800002a8:	fcc080e7          	jalr	-52(ra) # 80000270 <uart_putc>
        s++;
    800002ac:	0485                	add	s1,s1,1
    while (*s) {
    800002ae:	0004c503          	lbu	a0,0(s1)
    800002b2:	f96d                	bnez	a0,800002a4 <uart_puts+0x12>
    }
}
    800002b4:	60e2                	ld	ra,24(sp)
    800002b6:	6442                	ld	s0,16(sp)
    800002b8:	64a2                	ld	s1,8(sp)
    800002ba:	6105                	add	sp,sp,32
    800002bc:	8082                	ret

00000000800002be <memset>:
#include "string.h"
#include "types.h"

void* memset(void *dst, int c, uint n)  //填充
{
    800002be:	1141                	add	sp,sp,-16
    800002c0:	e422                	sd	s0,8(sp)
    800002c2:	0800                	add	s0,sp,16
  char *cdst = (char *) dst;
  int i;
  for(i = 0; i < n; i++){
    800002c4:	ca19                	beqz	a2,800002da <memset+0x1c>
    800002c6:	87aa                	mv	a5,a0
    800002c8:	1602                	sll	a2,a2,0x20
    800002ca:	9201                	srl	a2,a2,0x20
    800002cc:	00a60733          	add	a4,a2,a0
    cdst[i] = (char) c;
    800002d0:	00b78023          	sb	a1,0(a5)
  for(i = 0; i < n; i++){
    800002d4:	0785                	add	a5,a5,1
    800002d6:	fee79de3          	bne	a5,a4,800002d0 <memset+0x12>
  }
  return dst;
}
    800002da:	6422                	ld	s0,8(sp)
    800002dc:	0141                	add	sp,sp,16
    800002de:	8082                	ret

00000000800002e0 <memmove>:

void* memmove(void *dst, const void *src, uint n)   //复制
{
    800002e0:	1141                	add	sp,sp,-16
    800002e2:	e422                	sd	s0,8(sp)
    800002e4:	0800                	add	s0,sp,16
  const char *s;
  char *d;

  s = src;
  d = dst;
  if(s < d && s + n > d){
    800002e6:	02a5e563          	bltu	a1,a0,80000310 <memmove+0x30>
    s += n;
    d += n;
    while(n-- > 0)
      *--d = *--s;
  } else
    while(n-- > 0)
    800002ea:	fff6069b          	addw	a3,a2,-1
    800002ee:	ce11                	beqz	a2,8000030a <memmove+0x2a>
    800002f0:	1682                	sll	a3,a3,0x20
    800002f2:	9281                	srl	a3,a3,0x20
    800002f4:	0685                	add	a3,a3,1
    800002f6:	96ae                	add	a3,a3,a1
    800002f8:	87aa                	mv	a5,a0
      *d++ = *s++;
    800002fa:	0585                	add	a1,a1,1
    800002fc:	0785                	add	a5,a5,1
    800002fe:	fff5c703          	lbu	a4,-1(a1)
    80000302:	fee78fa3          	sb	a4,-1(a5)
    while(n-- > 0)
    80000306:	fed59ae3          	bne	a1,a3,800002fa <memmove+0x1a>

  return dst;
}
    8000030a:	6422                	ld	s0,8(sp)
    8000030c:	0141                	add	sp,sp,16
    8000030e:	8082                	ret
  if(s < d && s + n > d){
    80000310:	02061713          	sll	a4,a2,0x20
    80000314:	9301                	srl	a4,a4,0x20
    80000316:	00e587b3          	add	a5,a1,a4
    8000031a:	fcf578e3          	bgeu	a0,a5,800002ea <memmove+0xa>
    d += n;
    8000031e:	972a                	add	a4,a4,a0
    while(n-- > 0)
    80000320:	fff6069b          	addw	a3,a2,-1
    80000324:	d27d                	beqz	a2,8000030a <memmove+0x2a>
    80000326:	02069613          	sll	a2,a3,0x20
    8000032a:	9201                	srl	a2,a2,0x20
    8000032c:	fff64613          	not	a2,a2
    80000330:	963e                	add	a2,a2,a5
      *--d = *--s;
    80000332:	17fd                	add	a5,a5,-1
    80000334:	177d                	add	a4,a4,-1
    80000336:	0007c683          	lbu	a3,0(a5)
    8000033a:	00d70023          	sb	a3,0(a4)
    while(n-- > 0)
    8000033e:	fef61ae3          	bne	a2,a5,80000332 <memmove+0x52>
    80000342:	b7e1                	j	8000030a <memmove+0x2a>

0000000080000344 <strncmp>:

int strncmp(const char *p, const char *q, uint n)   //比较字符串
{
    80000344:	1141                	add	sp,sp,-16
    80000346:	e422                	sd	s0,8(sp)
    80000348:	0800                	add	s0,sp,16
  while(n > 0 && *p && *p == *q)
    8000034a:	ce11                	beqz	a2,80000366 <strncmp+0x22>
    8000034c:	00054783          	lbu	a5,0(a0)
    80000350:	cf89                	beqz	a5,8000036a <strncmp+0x26>
    80000352:	0005c703          	lbu	a4,0(a1)
    80000356:	00f71a63          	bne	a4,a5,8000036a <strncmp+0x26>
    n--, p++, q++;
    8000035a:	367d                	addw	a2,a2,-1
    8000035c:	0505                	add	a0,a0,1
    8000035e:	0585                	add	a1,a1,1
  while(n > 0 && *p && *p == *q)
    80000360:	f675                	bnez	a2,8000034c <strncmp+0x8>
  if(n == 0)
    return 0;
    80000362:	4501                	li	a0,0
    80000364:	a809                	j	80000376 <strncmp+0x32>
    80000366:	4501                	li	a0,0
    80000368:	a039                	j	80000376 <strncmp+0x32>
  if(n == 0)
    8000036a:	ca09                	beqz	a2,8000037c <strncmp+0x38>
  return (uchar)*p - (uchar)*q;
    8000036c:	00054503          	lbu	a0,0(a0)
    80000370:	0005c783          	lbu	a5,0(a1)
    80000374:	9d1d                	subw	a0,a0,a5
}
    80000376:	6422                	ld	s0,8(sp)
    80000378:	0141                	add	sp,sp,16
    8000037a:	8082                	ret
    return 0;
    8000037c:	4501                	li	a0,0
    8000037e:	bfe5                	j	80000376 <strncmp+0x32>

0000000080000380 <strncpy>:

char* strncpy(char *s, const char *t, int n)   //复制字符串
{
    80000380:	1141                	add	sp,sp,-16
    80000382:	e422                	sd	s0,8(sp)
    80000384:	0800                	add	s0,sp,16
  char *os;

  os = s;
  while(n-- > 0 && (*s++ = *t++) != 0)
    80000386:	87aa                	mv	a5,a0
    80000388:	86b2                	mv	a3,a2
    8000038a:	367d                	addw	a2,a2,-1
    8000038c:	00d05963          	blez	a3,8000039e <strncpy+0x1e>
    80000390:	0785                	add	a5,a5,1
    80000392:	0005c703          	lbu	a4,0(a1)
    80000396:	fee78fa3          	sb	a4,-1(a5)
    8000039a:	0585                	add	a1,a1,1
    8000039c:	f775                	bnez	a4,80000388 <strncpy+0x8>
    ;
  while(n-- > 0)
    8000039e:	873e                	mv	a4,a5
    800003a0:	9fb5                	addw	a5,a5,a3
    800003a2:	37fd                	addw	a5,a5,-1
    800003a4:	00c05963          	blez	a2,800003b6 <strncpy+0x36>
    *s++ = 0;
    800003a8:	0705                	add	a4,a4,1
    800003aa:	fe070fa3          	sb	zero,-1(a4)
  while(n-- > 0)
    800003ae:	40e786bb          	subw	a3,a5,a4
    800003b2:	fed04be3          	bgtz	a3,800003a8 <strncpy+0x28>
  return os;
    800003b6:	6422                	ld	s0,8(sp)
    800003b8:	0141                	add	sp,sp,16
    800003ba:	8082                	ret

00000000800003bc <printint>:
  uart_putc(c);
}

// 打印整数
static void printint(int xx, int base, int sign)
{
    800003bc:	7179                	add	sp,sp,-48
    800003be:	f406                	sd	ra,40(sp)
    800003c0:	f022                	sd	s0,32(sp)
    800003c2:	ec26                	sd	s1,24(sp)
    800003c4:	e84a                	sd	s2,16(sp)
    800003c6:	1800                	add	s0,sp,48
  char buf[16];
  int i;
  uint x;

  if(sign && (sign = xx < 0))
    800003c8:	c219                	beqz	a2,800003ce <printint+0x12>
    800003ca:	08054763          	bltz	a0,80000458 <printint+0x9c>
    x = -xx;
  else
    x = xx;
    800003ce:	2501                	sext.w	a0,a0
    800003d0:	4881                	li	a7,0
    800003d2:	fd040693          	add	a3,s0,-48

  i = 0;
    800003d6:	4701                	li	a4,0
  do {
    buf[i++] = digits[x % base];
    800003d8:	2581                	sext.w	a1,a1
    800003da:	00001617          	auipc	a2,0x1
    800003de:	e6e60613          	add	a2,a2,-402 # 80001248 <digits>
    800003e2:	883a                	mv	a6,a4
    800003e4:	2705                	addw	a4,a4,1
    800003e6:	02b577bb          	remuw	a5,a0,a1
    800003ea:	1782                	sll	a5,a5,0x20
    800003ec:	9381                	srl	a5,a5,0x20
    800003ee:	97b2                	add	a5,a5,a2
    800003f0:	0007c783          	lbu	a5,0(a5)
    800003f4:	00f68023          	sb	a5,0(a3)
  } while((x /= base) != 0);
    800003f8:	0005079b          	sext.w	a5,a0
    800003fc:	02b5553b          	divuw	a0,a0,a1
    80000400:	0685                	add	a3,a3,1
    80000402:	feb7f0e3          	bgeu	a5,a1,800003e2 <printint+0x26>

  if(sign)
    80000406:	00088c63          	beqz	a7,8000041e <printint+0x62>
    buf[i++] = '-';
    8000040a:	fe070793          	add	a5,a4,-32
    8000040e:	00878733          	add	a4,a5,s0
    80000412:	02d00793          	li	a5,45
    80000416:	fef70823          	sb	a5,-16(a4)
    8000041a:	0028071b          	addw	a4,a6,2

  while(--i >= 0)
    8000041e:	02e05763          	blez	a4,8000044c <printint+0x90>
    80000422:	fd040793          	add	a5,s0,-48
    80000426:	00e784b3          	add	s1,a5,a4
    8000042a:	fff78913          	add	s2,a5,-1
    8000042e:	993a                	add	s2,s2,a4
    80000430:	377d                	addw	a4,a4,-1
    80000432:	1702                	sll	a4,a4,0x20
    80000434:	9301                	srl	a4,a4,0x20
    80000436:	40e90933          	sub	s2,s2,a4
  uart_putc(c);
    8000043a:	fff4c503          	lbu	a0,-1(s1)
    8000043e:	00000097          	auipc	ra,0x0
    80000442:	e32080e7          	jalr	-462(ra) # 80000270 <uart_putc>
  while(--i >= 0)
    80000446:	14fd                	add	s1,s1,-1
    80000448:	ff2499e3          	bne	s1,s2,8000043a <printint+0x7e>
    putc(buf[i]);
}
    8000044c:	70a2                	ld	ra,40(sp)
    8000044e:	7402                	ld	s0,32(sp)
    80000450:	64e2                	ld	s1,24(sp)
    80000452:	6942                	ld	s2,16(sp)
    80000454:	6145                	add	sp,sp,48
    80000456:	8082                	ret
    x = -xx;
    80000458:	40a0053b          	negw	a0,a0
  if(sign && (sign = xx < 0))
    8000045c:	4885                	li	a7,1
    x = -xx;
    8000045e:	bf95                	j	800003d2 <printint+0x16>

0000000080000460 <printf>:
    putc(digits[x >> (sizeof(uint64) * 8 - 4)]);
}

// 格式化输出主函数
void printf(char *fmt, ...)
{
    80000460:	7171                	add	sp,sp,-176
    80000462:	f486                	sd	ra,104(sp)
    80000464:	f0a2                	sd	s0,96(sp)
    80000466:	eca6                	sd	s1,88(sp)
    80000468:	e8ca                	sd	s2,80(sp)
    8000046a:	e4ce                	sd	s3,72(sp)
    8000046c:	e0d2                	sd	s4,64(sp)
    8000046e:	fc56                	sd	s5,56(sp)
    80000470:	f85a                	sd	s6,48(sp)
    80000472:	f45e                	sd	s7,40(sp)
    80000474:	f062                	sd	s8,32(sp)
    80000476:	ec66                	sd	s9,24(sp)
    80000478:	1880                	add	s0,sp,112
    8000047a:	e40c                	sd	a1,8(s0)
    8000047c:	e810                	sd	a2,16(s0)
    8000047e:	ec14                	sd	a3,24(s0)
    80000480:	f018                	sd	a4,32(s0)
    80000482:	f41c                	sd	a5,40(s0)
    80000484:	03043823          	sd	a6,48(s0)
    80000488:	03143c23          	sd	a7,56(s0)
  va_list ap;
  int i, c;
  char *s;

  if (fmt == 0)
    8000048c:	16050f63          	beqz	a0,8000060a <printf+0x1aa>
    80000490:	8a2a                	mv	s4,a0
    return;

  va_start(ap, fmt);
    80000492:	00840793          	add	a5,s0,8
    80000496:	f8f43c23          	sd	a5,-104(s0)
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    8000049a:	00054503          	lbu	a0,0(a0)
    8000049e:	0005079b          	sext.w	a5,a0
    800004a2:	16050463          	beqz	a0,8000060a <printf+0x1aa>
    800004a6:	4481                	li	s1,0
    if(c != '%'){
    800004a8:	02500a93          	li	s5,37
      continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    switch(c){      // 格式化输出
    800004ac:	4bd5                	li	s7,21
    800004ae:	00001c17          	auipc	s8,0x1
    800004b2:	d42c0c13          	add	s8,s8,-702 # 800011f0 <printf_color+0xa36>
  uart_putc(c);
    800004b6:	4cc1                	li	s9,16
    putc(digits[x >> (sizeof(uint64) * 8 - 4)]);
    800004b8:	00001b17          	auipc	s6,0x1
    800004bc:	d90b0b13          	add	s6,s6,-624 # 80001248 <digits>
    800004c0:	a831                	j	800004dc <printf+0x7c>
  uart_putc(c);
    800004c2:	00000097          	auipc	ra,0x0
    800004c6:	dae080e7          	jalr	-594(ra) # 80000270 <uart_putc>
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    800004ca:	2485                	addw	s1,s1,1
    800004cc:	009a07b3          	add	a5,s4,s1
    800004d0:	0007c503          	lbu	a0,0(a5)
    800004d4:	0005079b          	sext.w	a5,a0
    800004d8:	12050963          	beqz	a0,8000060a <printf+0x1aa>
    if(c != '%'){
    800004dc:	ff5793e3          	bne	a5,s5,800004c2 <printf+0x62>
    c = fmt[++i] & 0xff;
    800004e0:	2485                	addw	s1,s1,1
    800004e2:	009a07b3          	add	a5,s4,s1
    800004e6:	0007c903          	lbu	s2,0(a5)
    if(c == 0)
    800004ea:	12090063          	beqz	s2,8000060a <printf+0x1aa>
    switch(c){      // 格式化输出
    800004ee:	0f590063          	beq	s2,s5,800005ce <printf+0x16e>
    800004f2:	f9d9079b          	addw	a5,s2,-99
    800004f6:	0ff7f793          	zext.b	a5,a5
    800004fa:	0efbed63          	bltu	s7,a5,800005f4 <printf+0x194>
    800004fe:	f9d9079b          	addw	a5,s2,-99
    80000502:	0ff7f713          	zext.b	a4,a5
    80000506:	0eebe763          	bltu	s7,a4,800005f4 <printf+0x194>
    8000050a:	00271793          	sll	a5,a4,0x2
    8000050e:	97e2                	add	a5,a5,s8
    80000510:	439c                	lw	a5,0(a5)
    80000512:	97e2                	add	a5,a5,s8
    80000514:	8782                	jr	a5
    case 'd':
      printint(va_arg(ap, int), 10, 1);    //十进制，显示符号
    80000516:	f9843783          	ld	a5,-104(s0)
    8000051a:	00878713          	add	a4,a5,8
    8000051e:	f8e43c23          	sd	a4,-104(s0)
    80000522:	4605                	li	a2,1
    80000524:	45a9                	li	a1,10
    80000526:	4388                	lw	a0,0(a5)
    80000528:	00000097          	auipc	ra,0x0
    8000052c:	e94080e7          	jalr	-364(ra) # 800003bc <printint>
      break;
    80000530:	bf69                	j	800004ca <printf+0x6a>
    case 'x':
      printint(va_arg(ap, int), 16, 0);    // 十六进制，不显示符号
    80000532:	f9843783          	ld	a5,-104(s0)
    80000536:	00878713          	add	a4,a5,8
    8000053a:	f8e43c23          	sd	a4,-104(s0)
    8000053e:	4601                	li	a2,0
    80000540:	85e6                	mv	a1,s9
    80000542:	4388                	lw	a0,0(a5)
    80000544:	00000097          	auipc	ra,0x0
    80000548:	e78080e7          	jalr	-392(ra) # 800003bc <printint>
      break;
    8000054c:	bfbd                	j	800004ca <printf+0x6a>
    case 'p':
      printptr(va_arg(ap, uint64));        // 指针
    8000054e:	f9843783          	ld	a5,-104(s0)
    80000552:	00878713          	add	a4,a5,8
    80000556:	f8e43c23          	sd	a4,-104(s0)
    8000055a:	0007b983          	ld	s3,0(a5)
  uart_putc(c);
    8000055e:	03000513          	li	a0,48
    80000562:	00000097          	auipc	ra,0x0
    80000566:	d0e080e7          	jalr	-754(ra) # 80000270 <uart_putc>
    8000056a:	07800513          	li	a0,120
    8000056e:	00000097          	auipc	ra,0x0
    80000572:	d02080e7          	jalr	-766(ra) # 80000270 <uart_putc>
    80000576:	8966                	mv	s2,s9
    putc(digits[x >> (sizeof(uint64) * 8 - 4)]);
    80000578:	03c9d793          	srl	a5,s3,0x3c
    8000057c:	97da                	add	a5,a5,s6
  uart_putc(c);
    8000057e:	0007c503          	lbu	a0,0(a5)
    80000582:	00000097          	auipc	ra,0x0
    80000586:	cee080e7          	jalr	-786(ra) # 80000270 <uart_putc>
  for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
    8000058a:	0992                	sll	s3,s3,0x4
    8000058c:	397d                	addw	s2,s2,-1
    8000058e:	fe0915e3          	bnez	s2,80000578 <printf+0x118>
    80000592:	bf25                	j	800004ca <printf+0x6a>
      break;
    case 's':
      if((s = va_arg(ap, char*)) == 0)    // 字符串
    80000594:	f9843783          	ld	a5,-104(s0)
    80000598:	00878713          	add	a4,a5,8
    8000059c:	f8e43c23          	sd	a4,-104(s0)
    800005a0:	0007b903          	ld	s2,0(a5)
    800005a4:	00090e63          	beqz	s2,800005c0 <printf+0x160>
        s = "(null)";
      for(; *s; s++)
    800005a8:	00094503          	lbu	a0,0(s2)
    800005ac:	dd19                	beqz	a0,800004ca <printf+0x6a>
  uart_putc(c);
    800005ae:	00000097          	auipc	ra,0x0
    800005b2:	cc2080e7          	jalr	-830(ra) # 80000270 <uart_putc>
      for(; *s; s++)
    800005b6:	0905                	add	s2,s2,1
    800005b8:	00094503          	lbu	a0,0(s2)
    800005bc:	f96d                	bnez	a0,800005ae <printf+0x14e>
    800005be:	b731                	j	800004ca <printf+0x6a>
        s = "(null)";
    800005c0:	00001917          	auipc	s2,0x1
    800005c4:	c2890913          	add	s2,s2,-984 # 800011e8 <printf_color+0xa2e>
      for(; *s; s++)
    800005c8:	02800513          	li	a0,40
    800005cc:	b7cd                	j	800005ae <printf+0x14e>
  uart_putc(c);
    800005ce:	8556                	mv	a0,s5
    800005d0:	00000097          	auipc	ra,0x0
    800005d4:	ca0080e7          	jalr	-864(ra) # 80000270 <uart_putc>
}
    800005d8:	bdcd                	j	800004ca <printf+0x6a>
      break;
    case '%':
      putc('%');
      break;
    case 'c':
      putc(va_arg(ap, uint));
    800005da:	f9843783          	ld	a5,-104(s0)
    800005de:	00878713          	add	a4,a5,8
    800005e2:	f8e43c23          	sd	a4,-104(s0)
  uart_putc(c);
    800005e6:	0007c503          	lbu	a0,0(a5)
    800005ea:	00000097          	auipc	ra,0x0
    800005ee:	c86080e7          	jalr	-890(ra) # 80000270 <uart_putc>
}
    800005f2:	bde1                	j	800004ca <printf+0x6a>
  uart_putc(c);
    800005f4:	8556                	mv	a0,s5
    800005f6:	00000097          	auipc	ra,0x0
    800005fa:	c7a080e7          	jalr	-902(ra) # 80000270 <uart_putc>
    800005fe:	854a                	mv	a0,s2
    80000600:	00000097          	auipc	ra,0x0
    80000604:	c70080e7          	jalr	-912(ra) # 80000270 <uart_putc>
}
    80000608:	b5c9                	j	800004ca <printf+0x6a>
      putc(c);
      break;
    }
  }
  va_end(ap);
    8000060a:	70a6                	ld	ra,104(sp)
    8000060c:	7406                	ld	s0,96(sp)
    8000060e:	64e6                	ld	s1,88(sp)
    80000610:	6946                	ld	s2,80(sp)
    80000612:	69a6                	ld	s3,72(sp)
    80000614:	6a06                	ld	s4,64(sp)
    80000616:	7ae2                	ld	s5,56(sp)
    80000618:	7b42                	ld	s6,48(sp)
    8000061a:	7ba2                	ld	s7,40(sp)
    8000061c:	7c02                	ld	s8,32(sp)
    8000061e:	6ce2                	ld	s9,24(sp)
    80000620:	614d                	add	sp,sp,176
    80000622:	8082                	ret

0000000080000624 <sprintf_simple>:
    console_puts(buf);
}

// 简单的sprintf实现（仅支持%d）
static void sprintf_simple(char *buf, const char *fmt, ...)
{
    80000624:	715d                	add	sp,sp,-80
    80000626:	ec22                	sd	s0,24(sp)
    80000628:	1000                	add	s0,sp,32
    8000062a:	e010                	sd	a2,0(s0)
    8000062c:	e414                	sd	a3,8(s0)
    8000062e:	e818                	sd	a4,16(s0)
    80000630:	ec1c                	sd	a5,24(s0)
    80000632:	03043023          	sd	a6,32(s0)
    80000636:	03143423          	sd	a7,40(s0)
    va_list ap;
    va_start(ap, fmt);
    8000063a:	fe843423          	sd	s0,-24(s0)
    
    // 仅处理 "\033[%d;%dH" 格式
    if(fmt[0] == '\033' && fmt[1] == '[') {     // 如果是颜色转义序列
    8000063e:	0005c703          	lbu	a4,0(a1)
    80000642:	47ed                	li	a5,27
    80000644:	00f70563          	beq	a4,a5,8000064e <sprintf_simple+0x2a>
        buf[pos++] = 'H';
        buf[pos] = '\0';
    }
    
    va_end(ap);
}
    80000648:	6462                	ld	s0,24(sp)
    8000064a:	6161                	add	sp,sp,80
    8000064c:	8082                	ret
    if(fmt[0] == '\033' && fmt[1] == '[') {     // 如果是颜色转义序列
    8000064e:	0015c703          	lbu	a4,1(a1)
    80000652:	05b00793          	li	a5,91
    80000656:	fef719e3          	bne	a4,a5,80000648 <sprintf_simple+0x24>
        int y = va_arg(ap, int);    // 获取y坐标
    8000065a:	00840793          	add	a5,s0,8
    8000065e:	fef43423          	sd	a5,-24(s0)
    80000662:	401c                	lw	a5,0(s0)
        int x = va_arg(ap, int);    // 获取x坐标
    80000664:	4418                	lw	a4,8(s0)
        buf[0] = '\033';
    80000666:	46ed                	li	a3,27
    80000668:	00d50023          	sb	a3,0(a0)
        buf[1] = '[';
    8000066c:	05b00693          	li	a3,91
    80000670:	00d500a3          	sb	a3,1(a0)
        if(y >= 10) buf[pos++] = '0' + y/10;
    80000674:	4625                	li	a2,9
        int pos = 2;
    80000676:	4689                	li	a3,2
        if(y >= 10) buf[pos++] = '0' + y/10;
    80000678:	00f65a63          	bge	a2,a5,8000068c <sprintf_simple+0x68>
    8000067c:	46a9                	li	a3,10
    8000067e:	02d7c6bb          	divw	a3,a5,a3
    80000682:	0306869b          	addw	a3,a3,48
    80000686:	00d50123          	sb	a3,2(a0)
    8000068a:	468d                	li	a3,3
        buf[pos++] = '0' + y%10;
    8000068c:	00d50633          	add	a2,a0,a3
    80000690:	45a9                	li	a1,10
    80000692:	02b7e7bb          	remw	a5,a5,a1
    80000696:	0307879b          	addw	a5,a5,48
    8000069a:	00f60023          	sb	a5,0(a2)
        buf[pos++] = ';';
    8000069e:	00268793          	add	a5,a3,2
    800006a2:	03b00593          	li	a1,59
    800006a6:	00b600a3          	sb	a1,1(a2)
        if(x >= 10) buf[pos++] = '0' + x/10;
    800006aa:	4625                	li	a2,9
    800006ac:	00e65c63          	bge	a2,a4,800006c4 <sprintf_simple+0xa0>
    800006b0:	97aa                	add	a5,a5,a0
    800006b2:	4629                	li	a2,10
    800006b4:	02c7463b          	divw	a2,a4,a2
    800006b8:	0306061b          	addw	a2,a2,48
    800006bc:	00c78023          	sb	a2,0(a5)
    800006c0:	00368793          	add	a5,a3,3
        buf[pos++] = '0' + x%10;
    800006c4:	00f506b3          	add	a3,a0,a5
    800006c8:	4629                	li	a2,10
    800006ca:	02c7673b          	remw	a4,a4,a2
    800006ce:	0307071b          	addw	a4,a4,48
    800006d2:	00e68023          	sb	a4,0(a3)
        buf[pos++] = 'H';
    800006d6:	0017871b          	addw	a4,a5,1
    800006da:	972a                	add	a4,a4,a0
    800006dc:	04800693          	li	a3,72
    800006e0:	00d70023          	sb	a3,0(a4)
        buf[pos] = '\0';
    800006e4:	2789                	addw	a5,a5,2
    800006e6:	953e                	add	a0,a0,a5
    800006e8:	00050023          	sb	zero,0(a0)
}
    800006ec:	bfb1                	j	80000648 <sprintf_simple+0x24>

00000000800006ee <console_putc>:
{
    800006ee:	1141                	add	sp,sp,-16
    800006f0:	e406                	sd	ra,8(sp)
    800006f2:	e022                	sd	s0,0(sp)
    800006f4:	0800                	add	s0,sp,16
    uart_putc(c);
    800006f6:	00000097          	auipc	ra,0x0
    800006fa:	b7a080e7          	jalr	-1158(ra) # 80000270 <uart_putc>
}
    800006fe:	60a2                	ld	ra,8(sp)
    80000700:	6402                	ld	s0,0(sp)
    80000702:	0141                	add	sp,sp,16
    80000704:	8082                	ret

0000000080000706 <console_puts>:
{
    80000706:	1101                	add	sp,sp,-32
    80000708:	ec06                	sd	ra,24(sp)
    8000070a:	e822                	sd	s0,16(sp)
    8000070c:	e426                	sd	s1,8(sp)
    8000070e:	1000                	add	s0,sp,32
    80000710:	84aa                	mv	s1,a0
    while(*s) {
    80000712:	00054503          	lbu	a0,0(a0)
    80000716:	c909                	beqz	a0,80000728 <console_puts+0x22>
    uart_putc(c);
    80000718:	00000097          	auipc	ra,0x0
    8000071c:	b58080e7          	jalr	-1192(ra) # 80000270 <uart_putc>
        s++;
    80000720:	0485                	add	s1,s1,1
    while(*s) {
    80000722:	0004c503          	lbu	a0,0(s1)
    80000726:	f96d                	bnez	a0,80000718 <console_puts+0x12>
}
    80000728:	60e2                	ld	ra,24(sp)
    8000072a:	6442                	ld	s0,16(sp)
    8000072c:	64a2                	ld	s1,8(sp)
    8000072e:	6105                	add	sp,sp,32
    80000730:	8082                	ret

0000000080000732 <clear_screen>:
{
    80000732:	1141                	add	sp,sp,-16
    80000734:	e406                	sd	ra,8(sp)
    80000736:	e022                	sd	s0,0(sp)
    80000738:	0800                	add	s0,sp,16
    console_puts("\033[2J");
    8000073a:	00001517          	auipc	a0,0x1
    8000073e:	b2650513          	add	a0,a0,-1242 # 80001260 <digits+0x18>
    80000742:	00000097          	auipc	ra,0x0
    80000746:	fc4080e7          	jalr	-60(ra) # 80000706 <console_puts>
    console_puts("\033[H");
    8000074a:	00001517          	auipc	a0,0x1
    8000074e:	b1e50513          	add	a0,a0,-1250 # 80001268 <digits+0x20>
    80000752:	00000097          	auipc	ra,0x0
    80000756:	fb4080e7          	jalr	-76(ra) # 80000706 <console_puts>
}
    8000075a:	60a2                	ld	ra,8(sp)
    8000075c:	6402                	ld	s0,0(sp)
    8000075e:	0141                	add	sp,sp,16
    80000760:	8082                	ret

0000000080000762 <clear_line>:
{
    80000762:	1141                	add	sp,sp,-16
    80000764:	e406                	sd	ra,8(sp)
    80000766:	e022                	sd	s0,0(sp)
    80000768:	0800                	add	s0,sp,16
    console_puts("\033[K");
    8000076a:	00001517          	auipc	a0,0x1
    8000076e:	b0650513          	add	a0,a0,-1274 # 80001270 <digits+0x28>
    80000772:	00000097          	auipc	ra,0x0
    80000776:	f94080e7          	jalr	-108(ra) # 80000706 <console_puts>
}
    8000077a:	60a2                	ld	ra,8(sp)
    8000077c:	6402                	ld	s0,0(sp)
    8000077e:	0141                	add	sp,sp,16
    80000780:	8082                	ret

0000000080000782 <goto_xy>:
{
    80000782:	7179                	add	sp,sp,-48
    80000784:	f406                	sd	ra,40(sp)
    80000786:	f022                	sd	s0,32(sp)
    80000788:	1800                	add	s0,sp,48
    sprintf_simple(buf, "\033[%d;%dH", y + 1, x + 1);   // 格式化输出坐标
    8000078a:	0015069b          	addw	a3,a0,1
    8000078e:	0015861b          	addw	a2,a1,1
    80000792:	00001597          	auipc	a1,0x1
    80000796:	ae658593          	add	a1,a1,-1306 # 80001278 <digits+0x30>
    8000079a:	fd040513          	add	a0,s0,-48
    8000079e:	00000097          	auipc	ra,0x0
    800007a2:	e86080e7          	jalr	-378(ra) # 80000624 <sprintf_simple>
    console_puts(buf);
    800007a6:	fd040513          	add	a0,s0,-48
    800007aa:	00000097          	auipc	ra,0x0
    800007ae:	f5c080e7          	jalr	-164(ra) # 80000706 <console_puts>
}
    800007b2:	70a2                	ld	ra,40(sp)
    800007b4:	7402                	ld	s0,32(sp)
    800007b6:	6145                	add	sp,sp,48
    800007b8:	8082                	ret

00000000800007ba <printf_color>:

// 颜色输出
void printf_color(int color, char *fmt, ...)
{
    800007ba:	711d                	add	sp,sp,-96
    800007bc:	f406                	sd	ra,40(sp)
    800007be:	f022                	sd	s0,32(sp)
    800007c0:	ec26                	sd	s1,24(sp)
    800007c2:	1800                	add	s0,sp,48
    800007c4:	84ae                	mv	s1,a1
    800007c6:	e010                	sd	a2,0(s0)
    800007c8:	e414                	sd	a3,8(s0)
    800007ca:	e818                	sd	a4,16(s0)
    800007cc:	ec1c                	sd	a5,24(s0)
    800007ce:	03043023          	sd	a6,32(s0)
    800007d2:	03143423          	sd	a7,40(s0)
    char color_code[16];
    
    // 构建颜色转义序列
    color_code[0] = '\033';
    800007d6:	47ed                	li	a5,27
    800007d8:	fcf40823          	sb	a5,-48(s0)
    color_code[1] = '[';
    800007dc:	05b00793          	li	a5,91
    800007e0:	fcf408a3          	sb	a5,-47(s0)
    color_code[2] = '0' + (color / 10);
    800007e4:	4729                	li	a4,10
    800007e6:	02e547bb          	divw	a5,a0,a4
    800007ea:	0307879b          	addw	a5,a5,48
    800007ee:	fcf40923          	sb	a5,-46(s0)
    color_code[3] = '0' + (color % 10);
    800007f2:	02e5653b          	remw	a0,a0,a4
    800007f6:	0305079b          	addw	a5,a0,48
    800007fa:	fcf409a3          	sb	a5,-45(s0)
    color_code[4] = 'm';
    800007fe:	06d00793          	li	a5,109
    80000802:	fcf40a23          	sb	a5,-44(s0)
    color_code[5] = '\0';
    80000806:	fc040aa3          	sb	zero,-43(s0)
    
    // 输出颜色转义序列
    console_puts(color_code);
    8000080a:	fd040513          	add	a0,s0,-48
    8000080e:	00000097          	auipc	ra,0x0
    80000812:	ef8080e7          	jalr	-264(ra) # 80000706 <console_puts>
    
    // 输出字符串
    console_puts(fmt);
    80000816:	8526                	mv	a0,s1
    80000818:	00000097          	auipc	ra,0x0
    8000081c:	eee080e7          	jalr	-274(ra) # 80000706 <console_puts>
    
    // 重置颜色
    console_puts("\033[0m");
    80000820:	00001517          	auipc	a0,0x1
    80000824:	a6850513          	add	a0,a0,-1432 # 80001288 <digits+0x40>
    80000828:	00000097          	auipc	ra,0x0
    8000082c:	ede080e7          	jalr	-290(ra) # 80000706 <console_puts>
    80000830:	70a2                	ld	ra,40(sp)
    80000832:	7402                	ld	s0,32(sp)
    80000834:	64e2                	ld	s1,24(sp)
    80000836:	6125                	add	sp,sp,96
    80000838:	8082                	ret
	...
