.section .text.entry
    .globl _entry
_entry:
    # 读取 hartid 并放入 tp，后续 S 态用 tp 取 hartid，避免再 csrr mhartid 触发非法指令
    csrr a0, mhartid
    mv tp, a0
    # 设置 M 态异常入口（可选）
    la t0, mtrap
    csrw mtvec, t0

    # 设置单核栈顶（后续可根据 tp 做偏移实现多核）
    la    sp, stack_top

    # 清 BSS
    la    t0, bss_start
    la    t1, bss_end
1:  bge   t0, t1, 2f
    sd    zero, 0(t0)
    addi  t0, t0, 8
    j     1b
2:
    # 进入 C 代码 start() 完成委托并跳到 main
    call  start
halt:
    j     halt

.section .bss
    .align 16
    .globl stack_bottom, stack_top
stack_bottom:
    .space 4096
stack_top:
