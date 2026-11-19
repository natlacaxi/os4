ENTRY(_start)
SECTIONS
{
    . = 0x40080000;
    .text   : { KEEP(*(.text.boot)) *(.text*) }
    .rodata : { *(.rodata*) }
    .data   : { *(.data*) }
    .bss (NOLOAD) : ALIGN(16) { __bss_start = .; *(.bss*) *(COMMON) . = ALIGN(16); __bss_end = .; }
}
