OUTPUT_ARCH(riscv)
ENTRY(_entry)

PHDRS
{
  text   PT_LOAD FLAGS(RX);
  rodata PT_LOAD FLAGS(R);
  data   PT_LOAD FLAGS(RW);
}

SECTIONS
{
  . = 0x80000000;

  .text : ALIGN(4) {
    *(.text.entry)          
    *(.text*)
  } :text
  PROVIDE(etext = .);

  .rodata : ALIGN(16) { *(.srodata*) *(.rodata*) } :rodata

  .data : ALIGN(16) { *(.sdata*) *(.data*) } :data

  .bss : ALIGN(16) {
    PROVIDE(bss_start = .);
    *(.sbss*)
    *(.bss*)
    . = ALIGN(16);
    PROVIDE(bss_end = .);
  } :data

  PROVIDE(end = .);
}
