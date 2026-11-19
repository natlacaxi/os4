/* TALCO-OS3 : entree ARM64 (EL1). Cible : QEMU virt, puis appareils ARM64. */
.section .text.boot
.global _start
_start:
    mov     x19, x0                 /* pointeur device tree */
    mrs     x1, mpidr_el1
    and     x1, x1, #0xff
    cbnz    x1, park                /* seul le coeur 0 demarre */
    ldr     x1, =stack_top
    mov     sp, x1
    ldr     x1, =__bss_start
    ldr     x2, =__bss_end
1:  cmp     x1, x2
    b.hs    2f
    str     xzr, [x1], #8
    b       1b
2:  mov     x0, x19
    mov     x1, xzr
    bl      kmain
park:
    wfe
    b       park

.section .bss
.align 4
.skip 16384
.global stack_top
stack_top:
