/* TALCO-OS3 : entree Multiboot i386 */
.set MAGIC,    0x1BADB002
.set FLAGS,    0x00000003
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:

.section .text
.global _start
_start:
    mov $stack_top, %esp
    push %ebx                      /* infos multiboot */
    push %eax                      /* magic */
    call kmain
    cli
1:  hlt
    jmp 1b

/* IRQ0 : timer PIT */
.global irq0_stub
irq0_stub:
    pusha
    cld
    call pit_tick
    mov $0x20, %al
    out %al, $0x20
    popa
    iret

/* exceptions CPU : on fige proprement au lieu d'un triple fault */
.global exc_stub
exc_stub:
    cli
1:  hlt
    jmp 1b

.section .note.GNU-stack,"",@progbits
